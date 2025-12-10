/**
 * @file main.c
 * @brief Main application file of the FM Radio
 *
 * This file contains the entry point of the program (`main`) and the control
 * logic of the whole device. It connects low-level drivers (twi, si4703,
 * oled, btns) into a working system
 *
 * @mainpage FM Radio Project Documentation
 *
 * @section intro_sec Introduction
 * This is firmware for an FM radio built on the AVR platform (ATmega328P)
 * The project uses the Si4703 module for FM reception and an OLED display
 * for showing information.
 *
 * @section features_sec Main features
 * - **Auto Scan:** Automatically searches for and stores the strongest stations at startup
 * - **Dual Mode:**
 *   - *MAN (Manual):* Tuning in 0.1 MHz steps
 *   - *AUT (Automatic):* Switching between stored stations
 * - **RDS:** Displaying station name and radio text (scrolling text)
 * - **Control:** 5 buttons (Up, Down, Volume+, Volume-, Mode)
 *
 * @author Mezera Vojtech, Moravec David, Mostecky Filip, Pavlicek Michal
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h> 

#include "twi.h"
#include "si4703.h"
#include "uart.h"
#include "oled.h"
#include "btns.h"

#ifndef F_CPU
#define F_CPU 16000000UL /**< @brief CPU frequency used for delay function */
#endif

// -- STATION MEMORY --
#define MAX_STANIC 20 /**< @brief Maximum number of stored stations */

/** @brief Array for storing frequencies of found stations */
uint16_t stanice[MAX_STANIC];

/** @brief Current number of found stations */
uint8_t pocet_stanic = 0;

/** @brief Index of the currently playing station in the array (-1 = none) */
int8_t aktualni_index = -1;

// -- STATE VARIABLES --
/** @brief Current volume level (0-15) */
uint8_t current_volume = 10; 

/** * @brief Tuning mode.
 * - 0 = AUT (switching between stored stations)
 * - 1 = MAN (manual frequency tuning)
 */
uint8_t rezim_manual = 0; 

/** @brief Minimum RSSI (signal strength) to store a station during scan */
#define MIN_RSSI_THRESHOLD 20 


// -- Helper function for frequency formatting --
/**
 * @brief Converts an integer frequency to a string with decimal point (e.g., "101.1")
 * @param freq Frequency (e.g. 1011)
 * @param buffer Output buffer where "101.1" will be written
 */
void format_freq(int freq, char* buffer) {
    int whole = freq / 10;
    int decimal = freq % 10;
    itoa(whole, buffer, 10);
    int len = 0;
    while(buffer[len] != '\0') len++;
    buffer[len] = '.';
    itoa(decimal, &buffer[len+1], 10);
}

// -- Helper function for printing number via UART --
/**
 * @brief Debug function for printing an integer via UART
 * @param num Number to be printed
 */
void print_int(int num) {
    char buffer[10];
    itoa(num, buffer, 10); 
    uart_puts(buffer);    
}

// -- Helper function for clearing RDS data --
/**
 * @brief Clears global buffers for RDS data and forces display update
 * Used when retuning to a different station so that old text does not stay on the display
 */
void clear_rds_data() {
    memset(rds_station_name, 0, 9);
    memset(rds_radio_text, 0, 65);
    rds_text_updated = 1; 
}

/*
 * UNIVERSAL DISPLAY FUNCTION
 * 1st line: [MOD] [FREQ]MHz VOL:[xx]
 */
/**
 * @brief Updates the content of the OLED display
 * * Draws the top status line (Mode, Frequency, Volume)
 * In the remaining area it either shows a system message (if provided)
 * or current RDS data (station name, radio text)
 *
 * @param status_label Text mode ("AUT", "MAN", "SCAN")
 * @param freq Current frequency to display
 * @param message System message (e.g. "Hledam..."), If NULL or empty, RDS data are displayed instead
 */
void update_display(const char* status_label, int freq, const char* message) {
    char topBar[32]; // Buffer for the whole top line
    char freqBuff[10];
    
    oled_clear_buffer(); 

    // -- 1st line: Status + Frequency + Volume --
    format_freq(freq, freqBuff);
    sprintf(topBar, "%s %sMHz VOL:%d", status_label, freqBuff, current_volume);

    oled_gotoxy(0, 0);
    oled_puts(topBar);

    // -- Decide what to show (System message vs. RDS) --
    if (message != NULL && message[0] != '\0') {
        oled_gotoxy(0, 3);
        oled_charMode(NORMALSIZE);
        oled_puts(message);
    } 
    else {
        // -- RDS data --
        oled_gotoxy(0, 2);
        oled_charMode(DOUBLESIZE); 
        if (rds_station_name[0] != '\0') {
            oled_puts(rds_station_name);
        } else {
            oled_puts("------");
        }
        oled_charMode(NORMALSIZE); 

        oled_gotoxy(0, 5);
        char line1[17];
        strncpy(line1, rds_radio_text, 16);
        line1[16] = '\0';
        oled_puts(line1);

        oled_gotoxy(0, 6);
        if (strlen(rds_radio_text) > 16) {
            char line2[17];
            strncpy(line2, rds_radio_text + 16, 16);
            line2[16] = '\0';
            oled_puts(line2);
        }
    }
    oled_display();
}

// -- AUTOMATIC TUNING (SCAN) --
/**
 * @brief Performs an automatic scan of the entire FM band.
 * The function repeatedly tunes upwards (SeekUp). If it finds a station
 * with RSSI higher than MIN_RSSI_THRESHOLD, it stores its frequency
 * into the `stanice` array. Progress is printed via UART and shown on OLED.
 */
void automaticke_ladeni(void) {
    uart_puts("Start Auto Scan...\r\n");
    pocet_stanic = 0;
    aktualni_index = -1;
    
    int current_freq = 875;
    si4703_setChannel(current_freq);
    _delay_ms(200); 

    update_display("SCAN", current_freq, "Hledam stanice...");
    
    while (pocet_stanic < MAX_STANIC) {
        int new_freq = si4703_seekUp();
        
        if (new_freq == 0 || new_freq <= current_freq) {
            break; 
        }
        current_freq = new_freq;

        int rssi = si4703_getRSSI();
        uart_puts("Freq: "); print_int(new_freq); 
        uart_puts(" RSSI: "); print_int(rssi);

        if (rssi < MIN_RSSI_THRESHOLD) {
            uart_puts(" -> SLABY\r\n");
            continue; 
        }
        uart_puts(" -> OK\r\n");

        stanice[pocet_stanic] = new_freq;
        update_display("SCAN", new_freq, "Ulozeno OK");
        
        pocet_stanic++;
        _delay_ms(100); 
    }

    if (pocet_stanic > 0) {
        aktualni_index = 0;
        si4703_setChannel(stanice[0]);
        rezim_manual = 0; // Automatically switch to AUT mode
    } else {
        si4703_setChannel(875);
        rezim_manual = 1; // If nothing was found, switch to MAN mode
    }
    clear_rds_data();
}

// -- Volume change --
/**
 * @brief Applies the current volume setting to the chip and updates the display
 * @param freq Current frequency (used for display redraw).
 */
void zmena_hlasitosti(int freq) {
    si4703_setVolume(current_volume);
    update_display(rezim_manual ? "MAN" : "AUT", freq, "");
}

/**
 * @brief Program entry point (Main Loop).
 * 1. Initializes peripherals (UART, I2C, OLED, buttons).
 * 2. Starts `automaticke_ladeni()`.
 * 3. Enters an infinite loop:
 *    - Reads buttons (S1-S5) and reacts to them.
 *    - Periodically (non-blocking) calls `si4703_process_rds()`.
 *    - Updates the display on state change or when RDS data change.
 */
int main(void) {
    // 1. Hardware initialization
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    sei();
    
    si4703_init();
    si4703_setVolume(current_volume);
    
    oled_init(OLED_DISP_ON); 
    oled_clrscr(); 
    buttons_init();

    // 2. Automatic scan at startup
    automaticke_ladeni();

    int current_freq = 875;
    if (pocet_stanic > 0) {
        current_freq = stanice[0];
    }

    update_display(rezim_manual ? "MAN" : "AUT", current_freq, pocet_stanic > 0 ? "Scan Hotov" : "Scan Prazdny");
    _delay_ms(1000);
    clear_rds_data();

    // Counter for RDS timing in the main loop
    uint8_t rds_poll_counter = 0;

    while (1) {
        // Button check (runs each loop = fast response)
        ButtonID btn = buttons_check_press();

        switch (btn) {
            case BTN_S1: // UP
                if (rezim_manual) {
                    current_freq += 1; 
                    if (current_freq > 1080) current_freq = 875; 
                    si4703_setChannel(current_freq);
                    update_display("MAN", current_freq, "");
                } 
                else {
                    if (pocet_stanic > 0) {
                        aktualni_index++;
                        if (aktualni_index >= pocet_stanic) aktualni_index = 0;
                        current_freq = stanice[aktualni_index];
                        si4703_setChannel(current_freq);
                        update_display("AUT", current_freq, "Nacitam...");
                    } else {
                        update_display("AUT", current_freq, "Zadna pamet");
                    }
                }
                clear_rds_data();
                break;

            case BTN_S2: // DOWN
                if (rezim_manual) {
                    current_freq -= 1;
                    if (current_freq < 875) current_freq = 1080;
                    si4703_setChannel(current_freq);
                    update_display("MAN", current_freq, "");
                } 
                else {
                    if (pocet_stanic > 0) {
                        aktualni_index--;
                        if (aktualni_index < 0) aktualni_index = pocet_stanic - 1;
                        current_freq = stanice[aktualni_index];
                        si4703_setChannel(current_freq);
                        update_display("AUT", current_freq, "Nacitam...");
                    } else {
                        update_display("AUT", current_freq, "Zadna pamet");
                    }
                }
                clear_rds_data();
                break;

            case BTN_S3: // VOL-
                if (current_volume > 0) {
                    current_volume--;
                    zmena_hlasitosti(current_freq);
                }
                break;

            case BTN_S4: // VOL+
                if (current_volume < 15) {
                    current_volume++;
                    zmena_hlasitosti(current_freq);
                }
                break;

            case BTN_S5: // MODE
                if (rezim_manual == 0) {
                    rezim_manual = 1;
                    update_display("MAN", current_freq, "Manual Mode");
                } else {
                    rezim_manual = 0;
                    update_display("AUT", current_freq, "Auto Mode");
                }
                _delay_ms(500);
                clear_rds_data();
                break;

            default:
                break;
        }

        /* * RDS timing logic:
         * One loop iteration includes 10 ms delay
         * RDS is called every 5th loop (5 x 10 ms = 50 ms)
         * This is sufficient for smooth reading and does not slow down button handling
         */
        rds_poll_counter++;
        if (rds_poll_counter > 5) {
            si4703_process_rds(); // Non-blocking, quickly checks RDS registers
            rds_poll_counter = 0;

            if (rds_text_updated) {
                update_display(rezim_manual ? "MAN" : "AUT", current_freq, "");
                rds_text_updated = 0;
            }
        }

        // Short delay for loop timing (defines button response speed)
        _delay_ms(10); 
    }
    return 0;
}