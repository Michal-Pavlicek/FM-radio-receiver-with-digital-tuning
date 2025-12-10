#ifndef ADC_BUTTONS_H_
#define ADC_BUTTONS_H_

/**
 * @file btns.h
 * @defgroup gemini_btns Buttons Library <btns.h>
 * @code #include "btns.h" @endcode
 *
 * @brief Library for handling analog buttons via the ADC.
 *
 * This module provides support for an analog keypad implemented
 * as a resistor ladder connected to a single ADC input.
 * It includes functions for:
 *  - ADC initialization,
 *  - raw button reading,
 *  - debounced, edge-detected button presses.
 * 
 * @author AI Assistant (Gemini)
 * @{
 */

#include <avr/io.h>

/**
 * @brief Logical identifiers of individual buttons.
 * * The enumeration maps ADC values to symbolic button names.
 * BTN_NONE (-1) means that no button is currently pressed.
 */
typedef enum {
    BTN_NONE = -1, /**< @brief No button pressed */
    BTN_S1   = 0,  /**< @brief Button 1 (left / lowest ADC value) */
    BTN_S2   = 1,  /**< @brief Button 2 */
    BTN_S3   = 2,  /**< @brief Button 3 */
    BTN_S4   = 3,  /**< @brief Button 4 */
    BTN_S5   = 4   /**< @brief Button 5 (right / highest ADC value) */
} ButtonID;


// --- Button interface ---

/**
 * @brief  Initialize the ADC for reading the analog keypad.
 * @return none
 * @note  Sets AVCC as reference and configures the ADC prescaler.
 */
void buttons_init(void);


/**
 * @brief  Read the current button state without debounce.
 * @return ID of the currently pressed button, or BTN_NONE.
 * @note   This function does not perform any debouncing or edge detection.
 *         It simply returns the button based on the instantaneous ADC value.
 */
ButtonID buttons_read_raw(void);


/**
 * @brief  Check for a new button press with debounce.
 * @return ID only on a new press event, otherwise BTN_NONE.
 * @note   This function:
 *  - detects state changes (edge detection),
 *  - applies a simple debounce using _delay_ms(),
 *  - returns the button ID only once per press (no repeats while held).
 */
ButtonID buttons_check_press(void);

/** @} */

#endif /* ADC_BUTTONS_H_ */