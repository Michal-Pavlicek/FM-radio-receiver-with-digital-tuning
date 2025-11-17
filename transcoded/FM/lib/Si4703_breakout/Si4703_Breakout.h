#ifndef si4703_breakout.h
#define si4703_breakout.h

#include <stdint.h>
#include <avr/io.h>

// I2C adresa čipu Si4703 (7-bit)
#define SI4703_ADDR 0x10

// Registry
#define POWERCFG    0x02
#define CHANNEL     0x03
#define SYSCONFIG1  0x04
#define SYSCONFIG2  0x05
#define STATUSRSSI  0x0A
#define READCHAN    0x0B
#define RDSB        0x0C
#define RDSD        0x0F

// Bity
#define TUNE    9
#define STC     7
#define SEEKUP  9
#define SEEK    8
#define SKMODE  10
#define SFBL    8
#define RDSR    15
#define RDS     12
#define DE      11
#define SPACE0  4

#define SEEK_UP   1
#define SEEK_DOWN 0

// Buffer 16 dvoubytových registrů
extern uint16_t si4703_registers[16];

// Popis HW (piny pro reset a SDIO)
typedef struct {
    volatile uint8_t* reset_ddr;
    volatile uint8_t* reset_port;
    uint8_t           reset_pin;

    volatile uint8_t* sdio_ddr;
    volatile uint8_t* sdio_port;
    uint8_t           sdio_pin;
} Si4703_HW;

// API
void si4703_init(const Si4703_HW* hw);
void si4703_setChannel(int channel_10khz);   // např. 1017 = 101.7 MHz
int  si4703_seekUp(void);
int  si4703_seekDown(void);
void si4703_setVolume(int volume_0_15);
int  si4703_getChannel(void);                // vrací 10x MHz (např. 973 = 97.3 MHz)
void si4703_readRDS(char* buffer8, uint16_t timeout_ms); // PS name 8 znaků nebo '' při timeoutu

#endif
