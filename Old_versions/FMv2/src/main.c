/*
 * Finalní verze FM Rádia
 * OLED Init musí být VŽDY před Si4703 Init!
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

// -- Pomocná funkce pro formátování frekvence (973 -> "97.3") --
void format_freq(int freq, char* buffer) {
    int whole = freq / 10;
    int decimal = freq % 10;
    itoa(whole, buffer, 10);
    int len = 0;
    while(buffer[len] != '\0') len++;
    buffer[len] = '.';
    itoa(decimal, &buffer[len+1], 10);
}

// -- Pomocná funkce pro výpis čísel přes UART --
void print_int(int num) {
    char buffer[10];
    itoa(num, buffer, 10); 
    uart_puts(buffer);    
}

// -- Funkce pro aktualizaci OLED displeje (Bezpečný update) --
void update_display(const char* status, int freq, const char* rds_text) {
    char buffer[16];

    // 1. Vymazat buffer v RAM (neposílá nic na displej, neruší rádio)
    oled_clear_buffer(); 

    // 2. Vykreslení do RAM bufferu
    
    // Řádek 0: Stav
    oled_gotoxy(0, 0);
    oled_puts("FM: ");
    oled_puts(status);

    // Řádek 2: Frekvence (Velké písmo)
    oled_gotoxy(0, 2);
    oled_charMode(DOUBLESIZE); 
    format_freq(freq, buffer);
    oled_puts(buffer);
    oled_puts(" MHz");
    oled_charMode(NORMALSIZE); 

    // Řádek 5: RDS
    if (rds_text != NULL && rds_text[0] != '\0') {
        oled_gotoxy(0, 5);
        oled_puts("RDS:");
        oled_gotoxy(0, 6);
        oled_puts(rds_text);
    }

    oled_display_block(0, 0, 128); // Header
    oled_display_block(0, 2, 128); // Freq horní půlka
    oled_display_block(0, 3, 128); // Freq dolní půlka

    if (rds_text != NULL && rds_text[0] != '\0') {
        oled_display_block(0, 5, 128); // RDS řádek 1
        oled_display_block(0, 6, 128); // RDS řádek 2
    }
}

int main(void) {
    // 1. Init UART
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    sei();

    uart_puts("Start System...\r\n");


    // 2. INIT RÁDIA 
    uart_puts("Init Si4703...\r\n");
    si4703_init();
    
    uart_puts("Radio OK.\r\n");

    si4703_setVolume(12);
    int current_freq = 973; 
    si4703_setChannel(current_freq);
    _delay_ms(200);

    // 3. INIT OLED
    oled_init(OLED_DISP_ON); 
    oled_clrscr(); 
    oled_puts("Startuji...");
    oled_display();
    
    _delay_ms(200); // Krátká pauza na ustálení napětí

    // Tlačítka (PB0 a PB1)
    DDRB &= ~((1 << PB0) | (1 << PB1));
    PORTB |= (1 << PB0) | (1 << PB1); // Pull-up

    // První vykreslení GUI
    update_display("Hraje", current_freq, "");

    char rdsBuffer[10];
    char lastRdsBuffer[10];
    lastRdsBuffer[0] = '\0';

    while (1) {
        // -- SEEK UP (PB0) --
        if (!(PINB & (1 << PB0))) {
            uart_puts("Seek UP...\r\n");
            update_display("Ladeni...", current_freq, "");
            
            current_freq = si4703_seekUp();
            
            if (current_freq == 0) {
                uart_puts("Nenalezeno.\r\n");
                update_display("Chyba", current_freq, "");
            } else {
                uart_puts("Naladeno: ");
                print_int(current_freq);
                uart_puts("\r\n");
                
                rdsBuffer[0] = '\0';
                lastRdsBuffer[0] = '\0';
                update_display("Hraje", current_freq, "");
            }
            _delay_ms(300);
        }

        // -- SEEK DOWN (PB1) --
        if (!(PINB & (1 << PB1))) {
            uart_puts("Seek DOWN...\r\n");
            update_display("Ladeni...", current_freq, "");
            
            current_freq = si4703_seekDown();
            
            if (current_freq == 0) {
                uart_puts("Nenalezeno.\r\n");
                update_display("Chyba", current_freq, "");
            } else {
                uart_puts("Naladeno: ");
                print_int(current_freq);
                uart_puts("\r\n");
                
                rdsBuffer[0] = '\0';
                lastRdsBuffer[0] = '\0';
                update_display("Hraje", current_freq, "");
            }
            _delay_ms(300);
        }

        // -- Čtení RDS --
        si4703_readRDS(rdsBuffer, 10);
        
        // Aktualizace displeje JEN když se změní text
        if (rdsBuffer[0] != '\0' && strcmp(rdsBuffer, lastRdsBuffer) != 0) {
            strcpy(lastRdsBuffer, rdsBuffer);
            uart_puts("RDS: ");
            uart_puts(rdsBuffer);
            uart_puts("\r\n");
            update_display("Hraje", current_freq, rdsBuffer);
        }
    }

    return 0;
}