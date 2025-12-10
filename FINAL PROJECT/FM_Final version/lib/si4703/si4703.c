/*
 * Si4703.c
 *
 * @brief Driver implementation for the Si4703 FM Tuner
 *        Provides basic control (init, tuning, seek, volume) and non-blocking RDS decoding.
 * @author AI Assistant (Gemini)
 */

#include "si4703.h"
#include "twi.h"
#include <util/delay.h>
#include <string.h> // For memset

// I2C address of the Si4703 (0x10)
#define SI4703_I2C_ADDR 0x10

// Register addresses
#define POWERCFG   0x02
#define CHANNEL    0x03
#define SYSCONFIG1 0x04
#define SYSCONFIG2 0x05
#define SYSCONFIG3 0x06
#define STATUSRSSI 0x0A
#define READCHAN   0x0B
#define RDSB       0x0D
#define RDSD       0x0F

// Bit positions within registers
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

// Global RDS buffers
char rds_station_name[9];
char rds_radio_text[65];
uint8_t rds_text_updated = 0;

// Shadow copy of all Si4703 register (16 register, 16 bits each)
static uint16_t si4703_registers[16];

// Internal function for writing all registers to Si4703
// Si4703 always starts writing at register 0x02
uint8_t si4703_updateRegisters(void) {
    twi_start();
    // Address + Write bit
    if (twi_write((SI4703_I2C_ADDR << 1) | TWI_WRITE) != 0) {
        twi_stop();
        return 1; // Error (NACK)
    }

    // Write registers 0x02 až 0x07
    for (int regSpot = 0x02; regSpot < 0x08; regSpot++) {
        uint8_t high_byte = si4703_registers[regSpot] >> 8;
        uint8_t low_byte = si4703_registers[regSpot] & 0x00FF;

        twi_write(high_byte);
        twi_write(low_byte);
    }

    twi_stop();
    return 0; // Success
}

// Internal function for reading all registers
// Si4703 starts reading at 0x0A, then 0X0B ... 0x0F, 0x00 ... 0x09
void si4703_readRegisters(void) {
    twi_start();
    // Address + Read bit
    twi_write((SI4703_I2C_ADDR << 1) | TWI_READ);

    // Read 32 bytes (16 words)
    // Si4703 vrací pořadí: 0x0A, 0x0B ... 0x0F, 0x00 ... 0x09
    for (int x = 0x0A; ; x++) {
        if (x == 0x10) x = 0; // Loop back to 0

        uint8_t high_byte;
        uint8_t low_byte;

        // Read high byte (ACK if not the very last byte)
        high_byte = twi_read(TWI_ACK);
        
        // Read low byte. If x == 0x09, it is the final byte -> send NACK
        if (x == 0x09) {
            low_byte = twi_read(TWI_NACK);
        } else {
            low_byte = twi_read(TWI_ACK);
        }

        si4703_registers[x] = (high_byte << 8) | low_byte;

        if (x == 0x09) break; // Done
    }
    twi_stop();
}

// Re-send current configuration (POWERCFG, SYSCONFIGx, CHANNEL)
void si4703_refresh(void) {
    si4703_updateRegisters();
}

void si4703_init(void) {
    // 1. Configure control pins
    SI4703_RST_DDR |= (1 << SI4703_RST_PIN);   // RST Output
    SI4703_SDIO_DDR |= (1 << SI4703_SDIO_PIN); // SDIO Output (for start-up sequence)

    // 2. Hardware reset sequence for I2C mode
    // SDIO must be LOW when RST goes from LOW to HIGH
    
    SI4703_SDIO_PORT &= ~(1 << SI4703_SDIO_PIN); // SDIO -> LOW
    SI4703_RST_PORT &= ~(1 << SI4703_RST_PIN);   // RST -> LOW
    
    // Ensure reset line stays low long enough for proper power-on reset
    _delay_ms(20); 

    SI4703_RST_PORT |= (1 << SI4703_RST_PIN);    // RST -> HIGH (Latches I2C mode)
    
    // Allow internal logic of the Si4703 to initialize after reset
    _delay_ms(20);

    // 3. Initialize TWI (I2C) interface
    twi_init();

    // 4. First communication - Enable oscillator
    si4703_readRegisters();
    // Enable oscillator (0x8100 in register 0x07)
    si4703_registers[0x07] = 0x8100;
    si4703_updateRegisters();

    _delay_ms(500); // Wait for crystall to stabilize

    // 5. Power up the radio
    si4703_readRegisters();
    si4703_registers[POWERCFG] = 0xC001; // Enable IC + DMUTE (Disable Mute - volitelné, zde enable IC)
    si4703_registers[SYSCONFIG1] |= (1 << RDS); // Enable RDS
    si4703_registers[SYSCONFIG1] |= (1 << DE);  // 50kHz Europe setup
    
    // --- TUNING AND SEEK CONFIGURATION ---
    
    // A) Set seek threshold (SEEKTH) - filter out noise-only channels
    si4703_registers[SYSCONFIG2] &= 0x00FF; // Clear upper byte (Old SEEKTH)
    si4703_registers[SYSCONFIG2] |= (25 << 8); // SEEKTH = 25
    
    // B) Set channel spacing (100 kHz for Europe)
    si4703_registers[SYSCONFIG2] |= (1 << SPACE0); 

    // C) Set initial volume (Lower 4 bity)
    si4703_registers[SYSCONFIG2] &= 0xFFF0;
    si4703_registers[SYSCONFIG2] |= 0x0001; 
    
    // D) Configure SKSNR (seek SNR threshold) in SYSCONFIG3
    //    Filters out strong noise that is not a valid station.
    si4703_registers[SYSCONFIG3] &= 0xFF0F; // Clears bits 7:4
    si4703_registers[SYSCONFIG3] |= (4 << 4); // SKSNR = 4
    
    si4703_updateRegisters();

    _delay_ms(110); // Max powerup time according to datasheet
}

void si4703_setChannel(uint16_t channel) {
    // Calculate channel frequency for Europe
    // Freq(MHz) = 0.100(Europe) * Channel + 87.5MHz
    int newChannel = channel;
    newChannel -= 875; 

    si4703_readRegisters();
    si4703_registers[CHANNEL] &= 0xFE00; // Clear channel bits
    si4703_registers[CHANNEL] |= newChannel; // Set new channel
    si4703_registers[CHANNEL] |= (1 << TUNE); // Set TUNE bit
    si4703_updateRegisters();

    // Wait for STC (Seek/Tune Complete)
    // Timeout counter as a safeguard
    uint16_t timeout = 0;
    while(1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1 << STC)) != 0) break;
        
        _delay_us(50);
        timeout++;
        if(timeout > 10000) break; // Break loop if hardware fail
    }

    si4703_readRegisters();
    si4703_registers[CHANNEL] &= ~(1 << TUNE); // Clear TUNE bit
    si4703_updateRegisters();

    // Wait until STC is cleared
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
    
    // Seek Mode setting (Wrap)
    si4703_registers[POWERCFG] |= (1 << SKMODE); 

    if (seekDirection == SEEK_DOWN) 
        si4703_registers[POWERCFG] &= ~(1 << SEEKUP);
    else 
        si4703_registers[POWERCFG] |= (1 << SEEKUP);

    si4703_registers[POWERCFG] |= (1 << SEEK); // Start seek
    si4703_updateRegisters();

    // Wait for STC (Seek complete)
    while(1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1 << STC)) != 0) break;
    }

    si4703_readRegisters();
    int valueSFBL = si4703_registers[STATUSRSSI] & (1 << SFBL);
    si4703_registers[POWERCFG] &= ~(1 << SEEK); // Clear seek bit
    si4703_updateRegisters();

    // Wait until STC is cleared
    while(1) {
        si4703_readRegisters();
        if ((si4703_registers[STATUSRSSI] & (1 << STC)) == 0) break;
    }

    if (valueSFBL) {
        return 0; // Seek failed
    }
    return si4703_getChannel();
}

int si4703_seekUp(void) {
    return si4703_seek(SEEK_UP);
}

int si4703_seekDown(void) {
    return si4703_seek(SEEK_DOWN);
}

int si4703_getRSSI(void) {
    si4703_readRegisters();
    // RSSI is store in lower 8 bits of STATUSRSSI register (0x0A)
    return si4703_registers[STATUSRSSI] & 0x00FF;
}

void si4703_setVolume(uint8_t volume) {
    si4703_readRegisters();
    if (volume > 15) volume = 15;
    si4703_registers[SYSCONFIG2] &= 0xFFF0; // Clear volume
    si4703_registers[SYSCONFIG2] |= volume; // Set volume
    si4703_updateRegisters();
}

/*
 * Non-blocking RDS processing function.
 *
 * Call this function in the main loop as often as reasonably possible
 * (for example every 40–50 ms using a timer or loop counter).
 * If no new RDS data are available, the function returns immediately
 * (no delays, does not block button handling or UI updates).
 */
void si4703_process_rds(void) {
    si4703_readRegisters();

    // 1) Check if new RDS data are ready (RDSR bit in STATUSRSSI / 0x0A)
    if (!(si4703_registers[STATUSRSSI] & (1 << 15))) {
 
        return; 
    }

    // 2) Decode RDS group
    uint16_t b = si4703_registers[0x0D]; // Block B
    uint16_t c = si4703_registers[0x0E]; // Block C
    uint16_t d = si4703_registers[0x0F]; // Block D
    
    // Determine group type and version
    uint8_t group_type = (b & 0xF000) >> 12;
    uint8_t version_b  = (b & 0x0800) >> 11; // 0 = A, 1 = B

    // --- Group 0A / 0B: Program Service name (PS – station name) ---
    if (group_type == 0) {
        uint8_t index = (b & 0x03); 
        
        char ch1 = (d >> 8);   // High byte
        char ch2 = (d & 0xFF); // Low byte

        if (ch1 >= 32 && ch1 <= 126) rds_station_name[index * 2]     = ch1;
        if (ch2 >= 32 && ch2 <= 126) rds_station_name[index * 2 + 1] = ch2;
        
        rds_station_name[8] = '\0';
        rds_text_updated = 1;
    }

    // --- Group 2A: RadioText (RT) ---
    else if (group_type == 2 && version_b == 0) {
        uint8_t text_index = (b & 0x0F);
        
        if (text_index < 16) {
            char c1 = (c >> 8);
            char c2 = (c & 0xFF);
            char d1 = (d >> 8);
            char d2 = (d & 0xFF);

            if (c1 >= 32) rds_radio_text[text_index * 4]     = c1;
            if (c2 >= 32) rds_radio_text[text_index * 4 + 1] = c2;
            if (d1 >= 32) rds_radio_text[text_index * 4 + 2] = d1;
            if (d2 >= 32) rds_radio_text[text_index * 4 + 3] = d2;
            
            rds_radio_text[64] = '\0';
            rds_text_updated = 1;
        }
    }
}