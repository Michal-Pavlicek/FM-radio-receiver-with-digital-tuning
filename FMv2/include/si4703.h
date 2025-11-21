/*
 * Si4703_Breakout_C.h
 * Přepis SparkFun Si4703 knihovny do čistého C pro AVR
 */

#ifndef SI4703_H
#define SI4703_H

#include <avr/io.h>
#include <stdint.h>

// -- Konfigurace Pinů (Upravte dle zapojení) ----------------
// Předpoklad: Arduino UNO (ATmega328P)
// SDIO je SDA (I2C Data) -> Na ATmega328P je to PC4
// RST je Reset pin -> Zde nastaveno na PD2 (Arduino Pin 2)

#define SI4703_RST_PORT PORTD
#define SI4703_RST_DDR  DDRD
#define SI4703_RST_PIN  PD2

#define SI4703_SDIO_PORT PORTC
#define SI4703_SDIO_DDR  DDRC
#define SI4703_SDIO_PIN  PC4  // SDA pin

// -- Definice pro ovládání ----------------------------------
#define SEEK_DOWN 0
#define SEEK_UP   1

// -- Funkční prototypy --------------------------------------

/**
 * @brief Inicializace Si4703 (Power On sekvence a nastavení I2C)
 */
void si4703_init(void);

/**
 * @brief Nastaví frekvenci kanálu (např. 973 pro 97.3 MHz)
 */
void si4703_setChannel(uint16_t channel);

/**
 * @brief Získá aktuální frekvenci kanálu
 */
int si4703_getChannel(void);

/**
 * @brief Automatické ladění nahoru
 * @return Frekvence kanálu nebo 0 při chybě
 */
int si4703_seekUp(void);

/**
 * @brief Automatické ladění dolů
 * @return Frekvence kanálu nebo 0 při chybě
 */
int si4703_seekDown(void);

/**
 * @brief Nastavení hlasitosti (0 - 15)
 */
void si4703_setVolume(uint8_t volume);

/**
 * @brief Přečte RDS data
 * @param buffer Ukazatel na char pole (musí mít alespoň 9 bytů)
 * @param timeout_ms Čas v ms, jak dlouho se má pokoušet číst
 */
void si4703_readRDS(char* buffer, uint16_t timeout_ms);

/**
 * @brief Pomocná funkce pro aktualizaci registrů (voláno interně)
 */
uint8_t si4703_updateRegisters(void);

/**
 * @brief Pomocná funkce pro čtení registrů (voláno interně)
 */
void si4703_readRegisters(void);
void si4703_refresh(void);
#endif