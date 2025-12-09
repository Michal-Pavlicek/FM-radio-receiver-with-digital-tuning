/*
 * Si4703_Breakout_C.h
 */
#ifndef SI4703_H
#define SI4703_H

#include <avr/io.h>
#include <stdint.h>

// -- Konfigurace Pinů --
// Uprav podle tvého zapojení
#define SI4703_RST_PORT PORTD
#define SI4703_RST_DDR  DDRD
#define SI4703_RST_PIN  PD2

#define SI4703_SDIO_PORT PORTC
#define SI4703_SDIO_DDR  DDRC
#define SI4703_SDIO_PIN  PC4

#define SEEK_DOWN 0
#define SEEK_UP   1

// -- GLOBÁLNÍ PROMĚNNÉ PRO RDS --
extern char rds_station_name[9]; 
extern char rds_radio_text[65];
extern uint8_t rds_text_updated;

// -- Funkční prototypy --
void si4703_init(void);
void si4703_setChannel(uint16_t channel);
int si4703_getChannel(void);
int si4703_seekUp(void);
int si4703_seekDown(void);
void si4703_setVolume(uint8_t volume);
int si4703_getRSSI(void);

// RDS Funkce (Non-blocking)
void si4703_process_rds(void);

// Pomocné
uint8_t si4703_updateRegisters(void);
void si4703_readRegisters(void);
void si4703_refresh(void);

#endif