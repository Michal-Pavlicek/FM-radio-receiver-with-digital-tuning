/**
 * @file main.c
 * @brief Hlavní aplikační soubor FM Rádia.
 *
 * Tento soubor obsahuje vstupní bod programu (`main`) a řídící logiku
 * celého zařízení. Propojuje nízkoúrovňové ovladače (twi, si4703, oled, btns)
 * do funkčního celku.
 *
 * @mainpage Dokumentace projektu FM Rádio
 *
 * @section intro_sec Úvod
 * Toto je firmware pro FM rádio postavené na platformě AVR (ATmega328P).
 * Projekt využívá modul Si4703 pro příjem FM vysílání a OLED displej pro
 * zobrazení informací.
 *
 * @section features_sec Hlavní funkce
 * - **Auto Scan:** Automatické vyhledání a uložení nejsilnějších stanic při startu.
 * - **Dual Mode:**
 * - *MAN (Manuální):* Ladění po 0.1 MHz krocích.
 * - *AUT (Automatický):* Přepínání mezi uloženými stanicemi.
 * - **RDS:** Zobrazování názvu stanice a radiotextu (běžící text).
 * - **Ovládání:** 5 tlačítek (Nahoru, Dolů, Hlasitost+, Hlasitost-, Mód).
 *
 * @author Mezera Vojtěch, Moravec David, Mostecký Filip, Pavlíček Michal
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
#define F_CPU 16000000UL /**< @brief Frekvence CPU pro delay funkce */
#endif

// -- PAMĚŤ STANIC --
#define MAX_STANIC 20 /**< @brief Maximální počet uložených stanic */

/** @brief Pole pro uložení frekvencí nalezených stanic */
uint16_t stanice[MAX_STANIC];

/** @brief Aktuální počet nalezených stanic */
uint8_t pocet_stanic = 0;

/** @brief Index aktuálně přehrávané stanice v poli (-1 = žádná) */
int8_t aktualni_index = -1;

// -- STAVOVÉ PROMĚNNÉ --
/** @brief Aktuální úroveň hlasitosti (0-15) */
uint8_t current_volume = 10; 

/** * @brief Režim ladění.
 * - 0 = AUT (Přepínání uložených stanic)
 * - 1 = MAN (Manuální ladění frekvence)
 */
uint8_t rezim_manual = 0; 

/** @brief Minimální RSSI (síla signálu) pro uložení stanice při skenu */
#define MIN_RSSI_THRESHOLD 20 


// -- Pomocná funkce pro formátování frekvence --
/**
 * @brief Převede celočíselnou frekvenci na textový řetězec s desetinnou tečkou.
 * @param freq Frekvence (např. 1011).
 * @param buffer Výstupní buffer, kam se zapíše "101.1".
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

// -- Pomocná funkce pro výpis čísel UART --
/**
 * @brief Ladicí funkce pro výpis čísla na UART.
 * @param num Číslo k vypsání.
 */
void print_int(int num) {
    char buffer[10];
    itoa(num, buffer, 10); 
    uart_puts(buffer);    
}

// -- Helper funkce pro vymazání RDS dat --
/**
 * @brief Vymaže globální buffery pro RDS data a vynutí aktualizaci displeje.
 * Používá se při přeladění stanice, aby nezůstal viset starý text.
 */
void clear_rds_data() {
    memset(rds_station_name, 0, 9);
    memset(rds_radio_text, 0, 65);
    rds_text_updated = 1; 
}

/*
 * UNIVERZÁLNÍ FUNKCE PRO DISPLEJ
 * 1. řádek: [MOD] [FREQ]MHz VOL:[xx]
 */
/**
 * @brief Aktualizuje obsah OLED displeje.
 * * Vykreslí horní stavový řádek (Mód, Frekvence, Hlasitost).
 * Ve zbytku displeje zobrazí buď systémovou hlášku (pokud existuje),
 * nebo aktuální RDS data (Název stanice, Radiotext).
 *
 * @param status_label Text módu ("AUT", "MAN", "SCAN").
 * @param freq Aktuální frekvence pro zobrazení.
 * @param message Systémová zpráva (např. "Hledam..."). Pokud je NULL nebo prázdná, zobrazí se RDS.
 */
void update_display(const char* status_label, int freq, const char* message) {
    char topBar[32]; // Buffer pro celý horní řádek
    char freqBuff[10];
    
    oled_clear_buffer(); 

    // -- 1. Řádek: Status + Frekvence + Hlasitost --
    format_freq(freq, freqBuff);
    sprintf(topBar, "%s %sMHz VOL:%d", status_label, freqBuff, current_volume);

    oled_gotoxy(0, 0);
    oled_puts(topBar);

    // -- Rozhodování co zobrazit (Systémová hláška vs. RDS) --
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

// -- AUTOMATICKÉ LADĚNÍ (SCAN) --
/**
 * @brief Provede automatické prohledání celého pásma (Scan).
 * * Funkce postupně ladí nahoru (SeekUp). Pokud najde stanici
 * s RSSI vyšším než MIN_RSSI_THRESHOLD, uloží ji do pole `stanice`.
 * Průběh je vypisován na UART a zobrazován na OLED.
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
        rezim_manual = 0; // Automaticky přepneme na AUT
    } else {
        si4703_setChannel(875);
        rezim_manual = 1; // Pokud nic nenajdeme, přepneme na MAN
    }
    clear_rds_data();
}

// -- Změna hlasitosti --
/**
 * @brief Aplikuje změnu hlasitosti do čipu a aktualizuje displej.
 * @param freq Aktuální frekvence (pro překreslení displeje).
 */
void zmena_hlasitosti(int freq) {
    si4703_setVolume(current_volume);
    update_display(rezim_manual ? "MAN" : "AUT", freq, "");
}

/**
 * @brief Vstupní bod programu (Main Loop).
 * * 1. Inicializuje periferie (UART, I2C, OLED, Buttons).
 * 2. Spustí `automaticke_ladeni()`.
 * 3. Vstupuje do nekonečné smyčky:
 * - Čte tlačítka (S1-S5) a reaguje na ně.
 * - Periodicky (neblokujícím způsobem) volá `si4703_process_rds()`.
 * - Aktualizuje displej při změně stavu nebo RDS dat.
 */
int main(void) {
    // 1. Inicializace hardwaru
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    sei();
    
    si4703_init();
    si4703_setVolume(current_volume);
    
    oled_init(OLED_DISP_ON); 
    oled_clrscr(); 
    buttons_init();

    // 2. AUTOMATICKÝ SKEN PŘI STARTU
    automaticke_ladeni();

    int current_freq = 875;
    if (pocet_stanic > 0) {
        current_freq = stanice[0];
    }

    update_display(rezim_manual ? "MAN" : "AUT", current_freq, pocet_stanic > 0 ? "Scan Hotov" : "Scan Prazdny");
    _delay_ms(1000);
    clear_rds_data();

    // Čítač pro časování RDS ve smyčce
    uint8_t rds_poll_counter = 0;

    while (1) {
        // Kontrola tlačítek (běží každou smyčku = rychle)
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

        /* * LOGIKA ČASOVÁNÍ RDS:
         * Celá smyčka má delay 10ms. 
         * RDS voláme jen každou 5. smyčku (5 * 10ms = 50ms).
         * To stačí na plynulé čtení, ale nezdržuje to tlačítka.
         */
        rds_poll_counter++;
        if (rds_poll_counter > 5) {
            si4703_process_rds(); // Toto nyní neblokuje, jen rychle mrkne do registrů
            rds_poll_counter = 0;

            if (rds_text_updated) {
                update_display(rezim_manual ? "MAN" : "AUT", current_freq, "");
                rds_text_updated = 0;
            }
        }

        // Krátký delay pro stabilizaci smyčky (určuje rychlost odezvy tlačítek)
        _delay_ms(10); 
    }
    return 0;
}