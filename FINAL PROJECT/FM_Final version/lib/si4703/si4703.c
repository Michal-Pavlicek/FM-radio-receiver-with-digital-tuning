/*
 * Si4703_Breakout_C.c
 *
 * @brief Implementace ovladače pro Si4703 FM Tuner.
 * @author AI Assistant (Gemini)
 */

#include "si4703.h"
#include "twi.h"
#include <util/delay.h>
#include <string.h> // Pro memset

// I2C Adresa Si4703 (0x10)
#define SI4703_I2C_ADDR 0x10

// Definice registrů
#define POWERCFG   0x02
#define CHANNEL    0x03
#define SYSCONFIG1 0x04
#define SYSCONFIG2 0x05
#define SYSCONFIG3 0x06 // PRIDANO: Potreba pro nastaveni SKSNR
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

// Globální buffery
char rds_station_name[9];
char rds_radio_text[65];
uint8_t rds_text_updated = 0;

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
    si4703_updateRegisters();
}

void si4703_init(void) {
    // 1. Nastavení pinů
    SI4703_RST_DDR |= (1 << SI4703_RST_PIN);   // RST Output
    SI4703_SDIO_DDR |= (1 << SI4703_SDIO_PIN); // SDIO Output (pro start sekvenci)

    // 2. Sekvence pro I2C mód (Hardware Reset)
    // SDIO musí být LOW ve chvíli, kdy jde RST z LOW na HIGH
    
    SI4703_SDIO_PORT &= ~(1 << SI4703_SDIO_PIN); // SDIO -> LOW
    SI4703_RST_PORT &= ~(1 << SI4703_RST_PIN);   // RST -> LOW
    
    // ZVĚTŠENO: Dlouhé čekání na vybití a ustálení
    _delay_ms(20); 

    SI4703_RST_PORT |= (1 << SI4703_RST_PIN);    // RST -> HIGH (Latch I2C mode)
    
    // ZVĚTŠENO: Čekání na nastartování vnitřní logiky čipu
    _delay_ms(20);

    // 3. Inicializace TWI (nyní převezme kontrolu nad SDIO/SDA pinem)
    twi_init();

    // 4. První komunikace - zapnutí oscilátoru
    si4703_readRegisters();
    // Enable oscilátoru (0x8100 na reg 0x07)
    si4703_registers[0x07] = 0x8100;
    si4703_updateRegisters();

    _delay_ms(500); // Čekání na krystal

    // 5. Zapnutí samotného rádia
    si4703_readRegisters();
    si4703_registers[POWERCFG] = 0xC001; // Enable IC + DMUTE (Disable Mute - volitelné, zde enable IC)
    si4703_registers[SYSCONFIG1] |= (1 << RDS); // Enable RDS
    si4703_registers[SYSCONFIG1] |= (1 << DE);  // 50kHz Europe setup
    
    // --- UPRAVA PRO OPTIMALIZACI LADENI ---
    
    // A) Nastavení prahu citlivosti pro Seek (SEEKTH)
    //    Výchozí je často 0, nastavíme na 25 (odfiltruje šum)
    si4703_registers[SYSCONFIG2] &= 0x00FF; // Smazat horní byte (staré SEEKTH)
    si4703_registers[SYSCONFIG2] |= (25 << 8); // Nastavit SEEKTH = 25
    
    // B) Nastavení odstupu kanálů (100kHz pro Evropu)
    si4703_registers[SYSCONFIG2] |= (1 << SPACE0); 

    // C) Hlasitost na minimum při startu (spodní 4 bity)
    si4703_registers[SYSCONFIG2] &= 0xFFF0;
    si4703_registers[SYSCONFIG2] |= 0x0001; 
    
    // D) Nastavení prahu SNR (SKSNR) v SYSCONFIG3
    //    Odfiltruje silné rušení, které není čistá stanice.
    si4703_registers[SYSCONFIG3] &= 0xFF0F; // Vyčistit bity 7:4
    si4703_registers[SYSCONFIG3] |= (4 << 4); // Nastavit SKSNR = 4
    
    si4703_updateRegisters();

    _delay_ms(110); // Max powerup time dle datasheetu
}

void si4703_setChannel(uint16_t channel) {
    // Výpočet frekvence pro registr
    // Freq(MHz) = 0.100(Europe) * Channel + 87.5MHz
    int newChannel = channel;
    newChannel -= 875; 

    si4703_readRegisters();
    si4703_registers[CHANNEL] &= 0xFE00; // Vyčistit bity kanálu
    si4703_registers[CHANNEL] |= newChannel; // Maskovat nový kanál
    si4703_registers[CHANNEL] |= (1 << TUNE); // Set TUNE bit
    si4703_updateRegisters();

    // Čekání na STC (Seek/Tune Complete)
    // Timeout counter pro jistotu
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

int si4703_getRSSI(void) {
    si4703_readRegisters();
    // RSSI je spodních 8 bitů registru STATUSRSSI (0x0A)
    return si4703_registers[STATUSRSSI] & 0x00FF;
}

void si4703_setVolume(uint8_t volume) {
    si4703_readRegisters();
    if (volume > 15) volume = 15;
    si4703_registers[SYSCONFIG2] &= 0xFFF0; // Clear volume
    si4703_registers[SYSCONFIG2] |= volume; // Set volume
    si4703_updateRegisters();
}

/* * OPRAVENÁ RDS FUNKCE (Non-blocking)
 * Volej tuto funkci v hlavní smyčce co nejčastěji (např. každých 40ms pomocí timeru).
 * Pokud nejsou nová data, funkce okamžitě skončí (return).
 * Žádné _delay_ms()!
 */
void si4703_process_rds(void) {
    si4703_readRegisters();

    // KROK 1: Kontrola, zda jsou data připravena (Bit RDSR v 0x0A)
    if (!(si4703_registers[STATUSRSSI] & (1 << 15))) {
        // Žádná nová data, končíme, nezdržujeme.
        return; 
    }

    // KROK 2: Zpracování dat
    uint16_t b = si4703_registers[0x0D]; // Registr Block B
    uint16_t c = si4703_registers[0x0E]; // Registr Block C
    uint16_t d = si4703_registers[0x0F]; // Registr Block D
    
    // Získání typu skupiny
    uint8_t group_type = (b & 0xF000) >> 12;
    uint8_t version_b  = (b & 0x0800) >> 11; // 0 = A (standard), 1 = B

    // --- SKUPINA 0A nebo 0B: NÁZEV STANICE (PS) ---
    if (group_type == 0) {
        uint8_t index = (b & 0x03); 
        
        char ch1 = (d >> 8);   // Horní byte
        char ch2 = (d & 0xFF); // Dolní byte

        if (ch1 >= 32 && ch1 <= 126) rds_station_name[index * 2]     = ch1;
        if (ch2 >= 32 && ch2 <= 126) rds_station_name[index * 2 + 1] = ch2;
        
        rds_station_name[8] = '\0';
        rds_text_updated = 1;
    }

    // --- SKUPINA 2A: RÁDIO TEXT (RT) ---
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