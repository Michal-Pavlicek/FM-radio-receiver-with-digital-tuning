/*
 * Si4703_Breakout_C.c
 */

#include "si4703.h"
#include "twi.h"
#include <util/delay.h>

// I2C Adresa Si4703 (0x10)
#define SI4703_I2C_ADDR 0x10

// Definice registrů
#define POWERCFG   0x02
#define CHANNEL    0x03
#define SYSCONFIG1 0x04
#define SYSCONFIG2 0x05
#define STATUSRSSI 0x0A
#define READCHAN   0x0B
#define RDSB       0x0D
#define RDSD       0x0F

// Bity registrů
#define SMUTE   15
#define DMUTE   14
#define SKMODE  10
#define SEEKUP  9
#define SEEK    8
#define TUNE    15
#define RDS     12
#define DE      11
#define SPACE1  5
#define SPACE0  4
#define RDSR    15
#define STC     14
#define SFBL    13
#define AFCRL   12
#define RDSS    11
#define STEREO  8

// Globální pole pro stínování registrů (16 registrů po 16 bitech)
static uint16_t si4703_registers[16];

// Interní funkce pro zápis na I2C sběrnici
// Si4703 začíná zápis vždy od registru 0x02
uint8_t si4703_updateRegisters(void) {
    twi_start();
    // Adresa + Write bit
    if (twi_write((SI4703_I2C_ADDR << 1) | TWI_WRITE) != 0) {
        twi_stop();
        return 1; // Chyba (NACK)
    }

    // Zapisujeme registry 0x02 až 0x07
    for (int regSpot = 0x02; regSpot < 0x08; regSpot++) {
        uint8_t high_byte = si4703_registers[regSpot] >> 8;
        uint8_t low_byte = si4703_registers[regSpot] & 0x00FF;

        twi_write(high_byte);
        twi_write(low_byte);
    }

    twi_stop();
    return 0; // Úspěch
}

// Interní funkce pro čtení všech registrů
// Si4703 začíná čtení od 0x0A, jede do 0x0F a pak loopuje na 0x00
void si4703_readRegisters(void) {
    twi_start();
    // Adresa + Read bit
    twi_write((SI4703_I2C_ADDR << 1) | TWI_READ);

    // Musíme přečíst 32 bytů (16 slov)
    // Si4703 vrací pořadí: 0x0A, 0x0B ... 0x0F, 0x00 ... 0x09
    for (int x = 0x0A; ; x++) {
        if (x == 0x10) x = 0; // Loop zpět na 0

        uint8_t high_byte;
        uint8_t low_byte;

        // Čteme High byte (ACK pokud to není úplně poslední byte komunikace)
        high_byte = twi_read(TWI_ACK);
        
        // Čteme Low byte. Pokud je x == 0x09, je to poslední byte, pošleme NACK
        if (x == 0x09) {
            low_byte = twi_read(TWI_NACK);
        } else {
            low_byte = twi_read(TWI_ACK);
        }

        si4703_registers[x] = (high_byte << 8) | low_byte;

        if (x == 0x09) break; // Jsme na konci
    }
    twi_stop();
}

void si4703_refresh(void) {
    // Znovu pošleme konfiguraci (Power, SysConfig, Channel)
    // Předpokládáme, že registry v poli si4703_registers[] jsou správné z minula
    si4703_updateRegisters();
}

void si4703_init(void) {
    // Nastavení pinů
    // RST jako Output
    SI4703_RST_DDR |= (1 << SI4703_RST_PIN);
    // SDIO jako Output (pro inicializační sekvenci)
    SI4703_SDIO_DDR |= (1 << SI4703_SDIO_PIN);

    // Sekvence pro uvedení do 2-wire (I2C) módu:
    // SDIO Low, RST Low -> RST High
    
    SI4703_SDIO_PORT &= ~(1 << SI4703_SDIO_PIN); // SDIO Low
    SI4703_RST_PORT &= ~(1 << SI4703_RST_PIN);   // RST Low
    
    _delay_ms(1); 

    SI4703_RST_PORT |= (1 << SI4703_RST_PIN);    // RST High
    
    _delay_ms(1);

    // Nyní přepneme SDIO pin zpět na vstup (resp. necháme I2C knihovnu, aby si ho přebrala)
    // TWI init se postará o konfiguraci SDA/SCL pinů
    twi_init();

    si4703_readRegisters();
    // Enable oscilátoru (0x8100 na reg 0x07)
    si4703_registers[0x07] = 0x8100;

    si4703_updateRegisters();

    _delay_ms(500); // Čekání na ustálení hodin

    si4703_readRegisters();
    si4703_registers[POWERCFG] = 0xC001; // Enable IC
    si4703_registers[SYSCONFIG1] |= (1 << RDS); // Enable RDS
    si4703_registers[SYSCONFIG1] |= (1 << DE);  // 50kHz Europe setup
    si4703_registers[SYSCONFIG2] |= (1 << SPACE0); // 100kHz channel spacing

    // Hlasitost na minimum
    si4703_registers[SYSCONFIG2] &= 0xFFF0;
    si4703_registers[SYSCONFIG2] |= 0x0001; 
    
    si4703_updateRegisters();

    _delay_ms(110); // Max powerup time
}

void si4703_setChannel(uint16_t channel) {
    // Výpočet frekvence pro registr
    // Freq(MHz) = 0.100(Europe) * Channel + 87.5MHz
    // Příklad: 97.3 MHz -> 973
    // 973 - 875 = 98
    
    int newChannel = channel;
    newChannel -= 875; 
    // newChannel /= 1; // spacing 100kHz (0.1 MHz), v integer logice x10

    si4703_readRegisters();
    si4703_registers[CHANNEL] &= 0xFE00; // Vyčistit bity kanálu
    si4703_registers[CHANNEL] |= newChannel; // Maskovat nový kanál
    si4703_registers[CHANNEL] |= (1 << TUNE); // Set TUNE bit
    si4703_updateRegisters();

    // Čekání na STC (Seek/Tune Complete)
    while(1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1 << STC)) != 0) break;
    }

    si4703_readRegisters();
    si4703_registers[CHANNEL] &= ~(1 << TUNE); // Clear TUNE bit
    si4703_updateRegisters();

    // Čekání na potvrzení vymazání STC
    while(1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1 << STC)) == 0) break;
    }
}

int si4703_getChannel(void) {
    si4703_readRegisters();
    int channel = si4703_registers[READCHAN] & 0x03FF;
    // Europe config
    channel += 875; 
    return channel;
}

static int si4703_seek(uint8_t seekDirection) {
    si4703_readRegisters();
    
    // Nastavení Seek Mode (Wrap)
    si4703_registers[POWERCFG] |= (1 << SKMODE); 

    if (seekDirection == SEEK_DOWN) 
        si4703_registers[POWERCFG] &= ~(1 << SEEKUP);
    else 
        si4703_registers[POWERCFG] |= (1 << SEEKUP);

    si4703_registers[POWERCFG] |= (1 << SEEK); // Start seek
    si4703_updateRegisters();

    // Poll STC
    while(1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1 << STC)) != 0) break;
    }

    si4703_readRegisters();
    int valueSFBL = si4703_registers[STATUSRSSI] & (1 << SFBL);
    si4703_registers[POWERCFG] &= ~(1 << SEEK); // Clear seek bit
    si4703_updateRegisters();

    while(1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1 << STC)) == 0) break;
    }

    if (valueSFBL) {
        return 0; // Failed
    }
    return si4703_getChannel();
}

int si4703_seekUp(void) {
    return si4703_seek(SEEK_UP);
}

int si4703_seekDown(void) {
    return si4703_seek(SEEK_DOWN);
}

void si4703_setVolume(uint8_t volume) {
    si4703_readRegisters();
    if (volume > 15) volume = 15;
    si4703_registers[SYSCONFIG2] &= 0xFFF0; // Clear volume
    si4703_registers[SYSCONFIG2] |= volume; // Set volume
    si4703_updateRegisters();
}

void si4703_readRDS(char* buffer, uint16_t timeout_ms) {
    // Náhrada millis() pomocí počítadla smyček a delay
    // Použijeme krok 40ms
    uint16_t steps = timeout_ms / 40;
    uint8_t completed[] = {0, 0, 0, 0};
    uint8_t completedCount = 0;

    for (uint16_t i = 0; i < steps; i++) {
        if (completedCount == 4) break;

        si4703_readRegisters();
        if (si4703_registers[STATUSRSSI] & (1 << RDSR)) {
            uint16_t b = si4703_registers[RDSB];
            int index = b & 0x03;
            
            if (!completed[index] && b < 500) {
                completed[index] = 1;
                completedCount++;
                
                char Dh = (si4703_registers[RDSD] & 0xFF00) >> 8;
                char Dl = (si4703_registers[RDSD] & 0x00FF);
                buffer[index * 2] = Dh;
                buffer[index * 2 + 1] = Dl;
            }
            _delay_ms(40); // Čekání na vymazání RDS bitu
        } else {
            _delay_ms(30); // Polling delay
        }
    }
    
    if (completedCount < 4) {
        buffer[0] = '\0'; // Pokud timeout, vyprázdnit
    } else {
        buffer[8] = '\0'; // Null terminate
    }
}