/*
 * Finalní verze FM Rádia s Auto Scan a RDS
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

#include "twi.h"
#include "si4703.h"
#include "uart.h"
#include "oled.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

// -- DEFINICE TLAČÍTEK --
#define BTN_UP   PB0
#define BTN_DOWN PB1
#define BTN_SCAN PB2

// -- PAMĚŤ STANIC --
#define MAX_STANIC 20
uint16_t stanice[MAX_STANIC];
uint8_t pocet_stanic = 0;
int8_t aktualni_index = -1;

// Práh pro uložení stanice (filtr šumu)
#define MIN_RSSI_THRESHOLD 20 

// -- Pomocná funkce pro formátování frekvence --
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
void print_int(int num) {
    char buffer[10];
    itoa(num, buffer, 10); 
    uart_puts(buffer);    
}

// -- Helper funkce pro vymazání RDS dat --
void clear_rds_data() {
    memset(rds_station_name, 0, 9); // Velikost pole z si4703.h
    memset(rds_radio_text, 0, 65);
    rds_text_updated = 1; 
}

/*
 * UNIVERZÁLNÍ FUNKCE PRO DISPLEJ
 * status: Text vlevo nahoře (FM, MEM, Ladeni...)
 * freq: Frekvence (např. 973)
 * message: Pokud NENÍ prázdné, zobrazí se uprostřed/dole (pro systémové hlášky).
 * Pokud JE prázdné (""), zobrazí se RDS data (Název stanice + Text).
 */
void update_display(const char* status, int freq, const char* message) {
    char freqBuff[16];
    
    oled_clear_buffer(); 

    // 1. Řádek: Status + Frekvence
    oled_gotoxy(0, 0);
    oled_puts(status);
    oled_puts(" ");
    format_freq(freq, freqBuff);
    oled_puts(freqBuff);
    oled_puts("MHz");

    // Rozhodování co zobrazit (Systémová hláška vs. RDS)
    if (message != NULL && message[0] != '\0') {
        // -- REŽIM SYSTÉMOVÉ ZPRÁVY (Ladění, Uloženo atd.) --
        oled_gotoxy(0, 3);
        oled_charMode(NORMALSIZE);
        oled_puts(message);
    } 
    else {
        // -- REŽIM RÁDIO (RDS data) --
        
        // A) Název stanice (velké písmo)
        oled_gotoxy(0, 2);
        oled_charMode(DOUBLESIZE); 
        if (rds_station_name[0] != '\0') {
            oled_puts(rds_station_name);
        } else {
            oled_puts("------");
        }
        oled_charMode(NORMALSIZE); 

        // B) Radiotext (běžící text - zalomený na 2 řádky)
        oled_gotoxy(0, 5);
        char line1[17];
        // Vezmeme prvních 16 znaků
        strncpy(line1, rds_radio_text, 16);
        line1[16] = '\0';
        oled_puts(line1);

        oled_gotoxy(0, 6);
        if (strlen(rds_radio_text) > 16) {
            char line2[17];
            // Vezmeme znaky od 16 dál
            strncpy(line2, rds_radio_text + 16, 16);
            line2[16] = '\0';
            oled_puts(line2);
        }
    }

    oled_display();
}

// -- AUTOMATICKÉ LADĚNÍ --
void automaticke_ladeni(void) {
    uart_puts("Start Auto Scan...\r\n");
    pocet_stanic = 0;
    aktualni_index = -1;
    
    int current_freq = 875;
    si4703_setChannel(current_freq);
    _delay_ms(200); 

    update_display("Skenuji", current_freq, "Hledam signal...");
    
    while (pocet_stanic < MAX_STANIC) {
        int new_freq = si4703_seekUp();
        
        if (new_freq == 0 || new_freq <= current_freq) {
            break; 
        }
        current_freq = new_freq;

        // Filtr signálu
        int rssi = si4703_getRSSI();
        uart_puts("Freq: "); print_int(new_freq); 
        uart_puts(" RSSI: "); print_int(rssi);

        if (rssi < MIN_RSSI_THRESHOLD) {
            uart_puts(" -> SLABY\r\n");
            continue; 
        }
        uart_puts(" -> OK\r\n");

        stanice[pocet_stanic] = new_freq;
        // Zobrazíme informaci o uložení
        update_display("Nalezeno", new_freq, "Ulozeno OK");
        
        pocet_stanic++;
        _delay_ms(100); 
    }

    if (pocet_stanic > 0) {
        aktualni_index = 0;
        si4703_setChannel(stanice[0]);
        clear_rds_data();
        update_display("Pamet", stanice[0], "Hotovo");
    } else {
        si4703_setChannel(875);
        update_display("Hotovo", 875, "Nic nenalezeno");
    }
    _delay_ms(1000);
}

int main(void) {
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    sei();
    
    si4703_init();
    si4703_setVolume(12);
    
    oled_init(OLED_DISP_ON); 
    oled_clrscr(); 
    oled_puts("Startuji...");
    oled_display();

    // Tlačítka
    DDRB &= ~((1 << BTN_UP) | (1 << BTN_DOWN) | (1 << BTN_SCAN));
    PORTB |= (1 << BTN_UP) | (1 << BTN_DOWN) | (1 << BTN_SCAN);

    int current_freq = 973;
    si4703_setChannel(current_freq);
    clear_rds_data();
    
    update_display("FM", current_freq, ""); // Prázdný string = ukaž RDS (pomlčky na začátku)

    while (1) {
        // -- SCAN (PB2) --
        if (!(PINB & (1 << BTN_SCAN))) {
            _delay_ms(50);
            if (!(PINB & (1 << BTN_SCAN))) {
                automaticke_ladeni();
                if (pocet_stanic > 0) current_freq = stanice[0];
                while(!(PINB & (1 << BTN_SCAN))); 
            }
        }

        // -- UP (PB0) --
        if (!(PINB & (1 << BTN_UP))) {
            _delay_ms(50);
            if (!(PINB & (1 << BTN_UP))) {
                if (pocet_stanic > 0) {
                    aktualni_index++;
                    if (aktualni_index >= pocet_stanic) aktualni_index = 0;
                    current_freq = stanice[aktualni_index];
                    si4703_setChannel(current_freq);
                    // Zobrazíme "Načítám" než naskočí RDS
                    update_display("MEM", current_freq, "Nacitam...");
                } else {
                    update_display("Ladeni", current_freq, "Seek UP...");
                    int r = si4703_seekUp();
                    if (r != 0) current_freq = r;
                    update_display("FM", current_freq, "");
                }
                clear_rds_data();
                _delay_ms(200);
            }
        }

        // -- DOWN (PB1) --
        if (!(PINB & (1 << BTN_DOWN))) {
            _delay_ms(50);
            if (!(PINB & (1 << BTN_DOWN))) {
                if (pocet_stanic > 0) {
                    aktualni_index--;
                    if (aktualni_index < 0) aktualni_index = pocet_stanic - 1;
                    current_freq = stanice[aktualni_index];
                    si4703_setChannel(current_freq);
                    update_display("MEM", current_freq, "Nacitam...");
                } else {
                    update_display("Ladeni", current_freq, "Seek DOWN...");
                    int r = si4703_seekDown();
                    if (r != 0) current_freq = r;
                    update_display("FM", current_freq, "");
                }
                clear_rds_data();
                _delay_ms(200);
            }
        }

        // -- RDS SMYČKA --
        // Zde voláme NOVOU funkci process_rds, ne starou readRDS!
        si4703_process_rds();

        if (rds_text_updated) {
            // Předáme prázdný string "", aby funkce věděla, že má kreslit RDS data
            update_display(pocet_stanic > 0 ? "MEM" : "FM", current_freq, "");
            rds_text_updated = 0;
        }
    }
    return 0;
}