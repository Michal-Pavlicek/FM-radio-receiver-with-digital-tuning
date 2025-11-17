#include "si4703_breakout.h"
#include "twi.h"
#include "gpio.h"
#include <util/delay.h>
#include <string.h>

uint16_t si4703_registers[16];

// --- Pomocné funkce ---
static void si4703_readRegisters(void) {
    uint8_t buffer[32];

    twi_start();
    twi_write((SI4703_ADDR << 1) | TWI_READ);
    for (int i = 0; i < 32; i++) {
        buffer[i] = twi_read(i < 31 ? TWI_ACK : TWI_NACK);
    }
    twi_stop();

    int x = 0x0A;
    for (int i = 0; i < 32; i += 2) {
        if (x == 0x10) x = 0;
        si4703_registers[x] = ((uint16_t)buffer[i] << 8) | buffer[i+1];
        if (x == 0x09) break;
        x++;
    }
}

static uint8_t si4703_updateRegisters(void) {
    twi_start();
    twi_write((SI4703_ADDR << 1) | TWI_WRITE);
    for (int reg = 0x02; reg < 0x08; reg++) {
        twi_write((uint8_t)(si4703_registers[reg] >> 8));
        twi_write((uint8_t)(si4703_registers[reg] & 0xFF));
    }
    twi_stop();
    return 1;
}

// --- API ---
void si4703_init(const Si4703_HW* hw) {
    // Reset sekvence pomocí GPIO
    gpio_mode_output(hw->sdio_ddr, hw->sdio_pin);
    gpio_write_low(hw->sdio_port, hw->sdio_pin);

    gpio_mode_output(hw->reset_ddr, hw->reset_pin);
    gpio_write_low(hw->reset_port, hw->reset_pin);
    _delay_ms(1);
    gpio_write_high(hw->reset_port, hw->reset_pin);
    _delay_ms(1);

    // Inicializace I2C
    twi_init();

    // Oscilátor enable
    si4703_readRegisters();
    si4703_registers[0x07] = 0x8100;
    si4703_updateRegisters();
    _delay_ms(500);

    // Power ON, RDS enable, Europe setup
    si4703_readRegisters();
    si4703_registers[POWERCFG] = 0x4001;
    si4703_registers[SYSCONFIG1] |= (1<<RDS);
    si4703_registers[SYSCONFIG1] |= (1<<DE);
    si4703_registers[SYSCONFIG2] |= (1<<SPACE0);
    si4703_registers[SYSCONFIG2] &= 0xFFF0;
    si4703_registers[SYSCONFIG2] |= 0x0001;
    si4703_updateRegisters();
    _delay_ms(110);
}

void si4703_setChannel(int channel_10khz) {
    int newChannel = channel_10khz * 10;
    newChannel -= 8750;
    newChannel /= 10;

    si4703_readRegisters();
    si4703_registers[CHANNEL] &= 0xFE00;
    si4703_registers[CHANNEL] |= (uint16_t)newChannel;
    si4703_registers[CHANNEL] |= (1<<TUNE);
    si4703_updateRegisters();

    while (1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1<<STC)) != 0) break;
    }

    si4703_readRegisters();
    si4703_registers[CHANNEL] &= ~(1<<TUNE);
    si4703_updateRegisters();

    while (1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1<<STC)) == 0) break;
    }
}

int si4703_seekUp(void) {
    si4703_readRegisters();
    si4703_registers[POWERCFG] |= (1<<SKMODE);
    si4703_registers[POWERCFG] |= (1<<SEEKUP);
    si4703_registers[POWERCFG] |= (1<<SEEK);
    si4703_updateRegisters();

    while (1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1<<STC)) != 0) break;
    }

    si4703_readRegisters();
    int valueSFBL = si4703_registers[STATUSRSSI] & (1<<SFBL);
    si4703_registers[POWERCFG] &= ~(1<<SEEK);
    si4703_updateRegisters();

    while (1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1<<STC)) == 0) break;
    }

    if (valueSFBL) return 0;
    return si4703_getChannel();
}

int si4703_seekDown(void) {
    si4703_readRegisters();
    si4703_registers[POWERCFG] |= (1<<SKMODE);
    si4703_registers[POWERCFG] &= ~(1<<SEEKUP);
    si4703_registers[POWERCFG] |= (1<<SEEK);
    si4703_updateRegisters();

    while (1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1<<STC)) != 0) break;
    }

    si4703_readRegisters();
    int valueSFBL = si4703_registers[STATUSRSSI] & (1<<SFBL);
    si4703_registers[POWERCFG] &= ~(1<<SEEK);
    si4703_updateRegisters();

    while (1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1<<STC)) == 0) break;
    }

    if (valueSFBL) return 0;
    return si4703_getChannel();
}

void si4703_setVolume(int volume_0_15) {
    si4703_readRegisters();
    if (volume_0_15 < 0)  volume_0_15 = 0;
    if (volume_0_15 > 15) volume_0_15 = 15;
    si4703_registers[SYSCONFIG2] &= 0xFFF0;
    si4703_registers[SYSCONFIG2] |= (uint16_t)volume_0_15;
    si4703_updateRegisters();
}

int si4703_getChannel(void) {
    si4703_readRegisters();
    int channel = si4703_registers[READCHAN] & 0x03FF;
    channel += 875;
    return channel;
}

void si4703_readRDS(char* buffer8, uint16_t timeout_ms) {
    memset(buffer8, 0, 9);
    uint8_t completed[4] = {0,0,0,0};
    uint8_t completedCount = 0;

    while (completedCount < 4 && timeout_ms > 0) {
        si4703_readRegisters();

        if (si4703_registers[STATUSRSSI] & (1<<RDSR)) {
            uint16_t b = si4703_registers[RDSB];
            int index = b & 0x03;
            if (index >= 0 && index < 4 && !completed[index] && b < 500) {
                char Dh = (char)((si4703_registers[RDSD] & 0xFF00) >> 8);
                char Dl = (char)(si4703_registers[RDSD] & 0x00FF);
                buffer8[index*2]     = Dh;
                buffer8[index*2 + 1] = Dl;
                completed[index] = 1;
                completedCount++;
            }
            _delay_ms(40);
            if (timeout_ms >= 40) timeout_ms -= 40; else timeout_ms = 0;
        } else {
            _delay_ms(30);
            if (timeout_ms >= 30) timeout_ms -= 30; else timeout_ms = 0;
        }
    }

    if (completedCount < 4) {
        buffer8[0] = '\0';
    } else {
        buffer8[8] = '\0';
    }
}
