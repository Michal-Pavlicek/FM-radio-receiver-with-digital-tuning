#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <uart.h>
#include "btns.h" // Vložení vaší nové knihovny

// --- Jednoduchá obsluha UART přímo v main pro účely testu ---
void uart_init_test(unsigned int baud) {
    unsigned int ubrr = F_CPU / 16 / baud - 1;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0); // Povolit vysílání
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit
}

void uart_print_test(char *str) {
    while (*str) {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = *str++;
    }
}

// --- Hlavní program ---
int main(void) {
    char buffer[40];

    // 1. Inicializace hardware
    uart_init_test(9600);
    buttons_init(); // Volání funkce z vaší knihovny
    
    // Pro jistotu nastavíme i LEDku, ať vidíme aktivitu
    DDRB |= (1 << DDB5); 

    uart_print_test("--- TEST KNIHOVNY ADC BUTTONS ---\r\n");

    while (1) {
        // 2. Použití knihovny: Zjistit, zda bylo zmáčknuto tlačítko
        // Funkce buttons_check_press() sama řeší debounce a vrací jen nové stisky
        ButtonID btn = buttons_check_press();

        if (btn != BTN_NONE) {
            // Rozsvítit LED při detekci
            PORTB |= (1 << PORTB5);

            // 3. Výpis výsledku podle toho, co vrátila knihovna
            switch (btn) {
                case BTN_S1:
                    uart_print_test("Stisknuto: S1 (Prepinani nahoru)\r\n");
                    break;
                case BTN_S2:
                    uart_print_test("Stisknuto: S2 (Prepinani dolu)\r\n");
                    break;
                case BTN_S3:
                    uart_print_test("Stisknuto: S3 (Hlasitost -)\r\n");
                    break;
                case BTN_S4:
                    uart_print_test("Stisknuto: S4 (Hlasitost +)\r\n");
                    break;
                case BTN_S5:
                    uart_print_test("Stisknuto: S5 (Mode Auto/Manual)\r\n");
                    break;
                default:
                    // Toto by se nemělo stát, pokud funguje BTN_NONE
                    sprintf(buffer, "Neznama klavesa ID: %d\r\n", btn);
                    uart_print_test(buffer);
                    break;
            }
            
            // Krátká pauza a zhasnutí LED
            _delay_ms(100);
            PORTB &= ~(1 << PORTB5);
        }
        
        // Ulehčíme procesoru, nemusí se ptát milionkrát za vteřinu
        _delay_ms(10);
    }

    return 0;
}