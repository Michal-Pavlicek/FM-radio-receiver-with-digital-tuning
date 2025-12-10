#include "btns.h"
#include <util/delay.h>

// Calibration values for buttons (ADC readings)
static const int adc_key_val[5] = {569, 637, 720, 828, 956};
static const int NUM_KEYS = 5;

// Internal variable to remember last key state
static int oldkey = -1;

// --- Internal helper: ADC read ---
static uint16_t internal_adc_read(uint8_t ch) {
    ch &= 0b00000111;               // Limit channel to ADC0-ADC7
    ADMUX = (ADMUX & 0xF8) | ch;    // Select ADC channel, keep reference settings
    ADCSRA |= (1 << ADSC);          // Start conversion
    while (ADCSRA & (1 << ADSC));   // Wait until conversion complete
    return (ADC);                   // Return ADC value
}

// --- Button driver – public functions ---

void buttons_init(void) {
    // Configure ADC: AVCC reference, prescaler = 128
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

ButtonID buttons_read_raw(void) {
    // Read analog value from channel 0 (keypad input)
    uint16_t input = internal_adc_read(0);
    
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

    // Edge detection: react only to changes compared to previous state
    if (key != oldkey) {
        _delay_ms(50); // Debounce delay
        
        // Re-read after debounce interval
        key = buttons_read_raw();
        
        if (key != oldkey) {
            oldkey = key;
            if (key != BTN_NONE) {
                return key;  // Return only on a new key press
            }
        }
    }
    return BTN_NONE; // No new key event detected
}