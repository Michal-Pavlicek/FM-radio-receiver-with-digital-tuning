/*
 * Si4703_Breakout_C.h
 */
#ifndef SI4703_H
#define SI4703_H

/**
 * @file si4703.h
 * @defgroup gemini_si4703 Si4703 FM Radio Library <si4703.h>
 * @code #include "si4703.h" @endcode
 *
 * @brief Ovladač pro FM tuner Si4703.
 *
 * Knihovna zajišťuje komunikaci s čipem Si4703 přes I2C sběrnici.
 * Podporuje nastavení frekvence, automatické ladění (Seek), ovládání
 * hlasitosti a neblokující čtení RDS dat (název stanice, radiotext).
 *
 * @author AI Assistant (Gemini)
 * @{
 */

#include <avr/io.h>
#include <stdint.h>

// -- Konfigurace Pinů --
// Uprav podle tvého zapojení
#define SI4703_RST_PORT PORTD /**< @brief Port pro Reset pin */
#define SI4703_RST_DDR  DDRD  /**< @brief DDR registr pro Reset pin */
#define SI4703_RST_PIN  PD2   /**< @brief Pin číslo pro Reset */

#define SI4703_SDIO_PORT PORTC /**< @brief Port pro SDIO (SDA) pin */
#define SI4703_SDIO_DDR  DDRC  /**< @brief DDR registr pro SDIO (SDA) pin */
#define SI4703_SDIO_PIN  PC4   /**< @brief Pin číslo pro SDIO (SDA) */

#define SEEK_DOWN 0 /**< @brief Směr ladění dolů */
#define SEEK_UP   1 /**< @brief Směr ladění nahoru */

// -- GLOBÁLNÍ PROMĚNNÉ PRO RDS --
/** @brief Buffer pro název stanice (8 znaků + null terminator) */
extern char rds_station_name[9]; 
/** @brief Buffer pro radiotext (64 znaků + null terminator) */
extern char rds_radio_text[65];
/** @brief Flag indikující aktualizaci RDS textu (1 = nová data) */
extern uint8_t rds_text_updated;

// -- Funkční prototypy --

/**
 * @brief  Inicializuje čip Si4703.
 * @return none
 * @note   Provádí hardwarový reset sekvenci, nastavení I2C módu,
 * zapnutí oscilátoru a základní konfiguraci pro Evropu.
 * Funkce trvá cca 1 sekundu kvůli nutným prodlevám.
 */
void si4703_init(void);


/**
 * @brief  Naladí rádio na specifickou frekvenci.
 * @param  channel Frekvence v desítkách kHz (např. 1011 pro 101.1 MHz).
 * @return none
 */
void si4703_setChannel(uint16_t channel);


/**
 * @brief  Přečte aktuálně naladěnou frekvenci.
 * @return Frekvence v desítkách kHz (např. 895 pro 89.5 MHz).
 */
int si4703_getChannel(void);


/**
 * @brief  Spustí automatické ladění (Seek) směrem nahoru.
 * @return Nově naladěná frekvence nebo 0 pokud ladění selhalo (přeplnění pásma).
 */
int si4703_seekUp(void);


/**
 * @brief  Spustí automatické ladění (Seek) směrem dolů.
 * @return Nově naladěná frekvence nebo 0 pokud ladění selhalo.
 */
int si4703_seekDown(void);


/**
 * @brief  Nastaví hlasitost výstupu sluchátek.
 * @param  volume Úroveň hlasitosti (0-15).
 * @return none
 */
void si4703_setVolume(uint8_t volume);


/**
 * @brief  Získá sílu signálu (RSSI).
 * @return Hodnota RSSI (Received Signal Strength Indicator). Vyšší = silnější.
 */
int si4703_getRSSI(void);


/**
 * @brief  Zpracuje RDS data (Non-blocking).
 * @return none
 * @note   Tuto funkci je nutné volat cyklicky v hlavní smyčce (např. každých 40ms).
 * Pokud jsou dostupná nová RDS data, aktualizuje globální buffery
 * `rds_station_name` a `rds_radio_text`.
 */
void si4703_process_rds(void);


// Pomocné
/**
 * @brief  Odešle obsah stínových registrů do čipu přes I2C.
 * @return 0 při úspěchu (ACK), 1 při chybě (NACK).
 */
uint8_t si4703_updateRegisters(void);


/**
 * @brief  Přečte všechny registry z čipu do lokálního pole.
 * @return none
 */
void si4703_readRegisters(void);


/**
 * @brief  Znovu aplikuje konfiguraci registrů (Power, SysConfig, Channel).
 * @return none
 */
void si4703_refresh(void);

/** @} */

#endif