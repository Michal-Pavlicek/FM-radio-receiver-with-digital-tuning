#include "btns.h"
#include <util/delay.h>

// Vaše zkalibrované hodnoty
static const int adc_key_val[5] = {550, 630, 700, 800, 900};
static const int NUM_KEYS = 5;

// Interní proměnná pro paměť posledního stisku
static int oldkey = -1;

// --- Interní pomocná funkce pro čtení ADC (není v .h souboru) ---
static uint16_t internal_adc_read(uint8_t ch) {
    ch &= 0b00000111;
    ADMUX = (ADMUX & 0xF8) | ch;
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return (ADC);
}

// --- Implementace veřejných funkcí ---

void buttons_init(void) {
    // Nastavení ADC (AVCC reference, Prescaler 128)
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

ButtonID buttons_read_raw(void) {
    uint16_t input = internal_adc_read(0); // Čteme kanál 0
    
    int k;
    for (k = 0; k < NUM_KEYS; k++) {
        if (input < adc_key_val[k]) {
            return (ButtonID)k;
        }
    }
    return BTN_NONE;
}

ButtonID buttons_check_press(void) {
    ButtonID key = buttons_read_raw();

    // Detekce náběžné hrany (změna oproti minule)
    if (key != oldkey) {
        _delay_ms(50); // Debounce
        
        // Kontrolní čtení
        key = buttons_read_raw();
        
        if (key != oldkey) {
            oldkey = key;
            if (key != BTN_NONE) {
                return key; // Vrátíme stisknuté tlačítko
            }
        }
    }
    return BTN_NONE; // Nic nového se nestalo
}