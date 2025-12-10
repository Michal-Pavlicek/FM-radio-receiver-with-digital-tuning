/*
 * Si4703.h
 */
#ifndef SI4703_H
#define SI4703_H

/**
 * @file si4703.h
 * @defgroup gemini_si4703 Si4703 FM Radio Library <si4703.h>
 * @code #include "si4703.h" @endcode
 *
 * @brief Driver for FM tuner Si4703.
 *
 * This library handles communication with the Si4703 chip via the I2C bus.
 * It provides functions for frequency tuning, automatic seek,
 * volume control and non-blocking RDS reception (program service name and radio text).
 *
 * @author AI Assistant (Gemini)
 * @{
 */

#include <avr/io.h>
#include <stdint.h>

// -- Pin configuration --
#define SI4703_RST_PORT PORTD /**< @brief Port for Reset pin */
#define SI4703_RST_DDR  DDRD  /**< @brief DDR register for Reset pin */
#define SI4703_RST_PIN  PD2   /**< @brief Pin číslo pro Reset */

#define SI4703_SDIO_PORT PORTC /**< @brief Port for SDIO (SDA) pin */
#define SI4703_SDIO_DDR  DDRC  /**< @brief DDR register for SDIO (SDA) pin */
#define SI4703_SDIO_PIN  PC4   /**< @brief Pin number for SDIO (SDA) */

#define SEEK_DOWN 0 /**< @brief Seek downward in frequency band */
#define SEEK_UP   1 /**< @brief Seek upwards in frequency band */

// -- GLOBAL RDS VALUES --
/** @brief Buffer for the station name (8 characters + null terminator) */
extern char rds_station_name[9]; 
/** @brief Buffer for radio text (64 characters + null terminator) */
extern char rds_radio_text[65];
/** @brief Flag indicating that RDS text has been updated (1 = new data) */
extern uint8_t rds_text_updated;

// -- Funkční prototypy --

/**
 * @brief  Initialize chip Si4703.
 * @return none
 * @note   Performs hardware reset sequence, enables I2C mode,
 *         starts the crystal oscillator, and applies basic configuration
 *         for European FM band.
 */
void si4703_init(void);


/**
 * @brief  Tune the radio to a specific frequency.
 * @param  channel Frequency in 0.1 MHz units (e.g. 1011 for 101.1 MHz).
 * @return none
 */
void si4703_setChannel(uint16_t channel);


/**
 * @brief  Read back the currently tuned frequency.
 * @return Frequency in 0.1 MHz units (e.g. 895 pro 89.5 MHz).
 */
int si4703_getChannel(void);


/**
 * @brief  Start automatic seek upwards.
 * @return Newly tuned frequency or 0 if seek failed (band overflow).
 */
int si4703_seekUp(void);


/**
 * @brief  Start automatic seek downwards.
 * @return Newly tuned frequency or 0 if seek failed.
 */
int si4703_seekDown(void);


/**
 * @brief  Set output volume.
 * @param  volume Volume level in range 0-15.
 * @return none
 */
void si4703_setVolume(uint8_t volume);


/**
 * @brief  Read received signal strength (RSSI).
 * @return RSSI value (Received Signal Strength Indicator).¨
 * @note   Higher values indicate stronger signal.
 */
int si4703_getRSSI(void);


/**
 * @brief  Process RDS data (Non-blocking).
 * @return none
 * @note   This function must be called periodically from the main loop
 *         (for example every ~40 ms). If new RDS data are available,
 *         it updates the global buffers `rds_station_name`
 *         and `rds_radio_text` and sets `rds_text_updated`.
 */
void si4703_process_rds(void);


// Helper functions
/**
 * @brief  Send the contents of shadow registers to the chip via I2C.
 * @return 0 on success (ACK received), 1 on error (NACK).
 */
uint8_t si4703_updateRegisters(void);


/**
 * @brief  Read all registers from the chip into local array.
 * @return none
 */
void si4703_readRegisters(void);


/**
 * @brief  Re-apply register configuration (power, system configuration, channel).
 * @return none
 */
void si4703_refresh(void);

/** @} */

#endif