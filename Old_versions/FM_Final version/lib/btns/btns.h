#ifndef ADC_BUTTONS_H_
#define ADC_BUTTONS_H_

/**
 * @file btns.h
 * @defgroup gemini_btns Buttons Library <btns.h>
 * @code #include "btns.h" @endcode
 *
 * @brief Knihovna pro obsluhu analogových tlačítek přes ADC.
 *
 * Tato knihovna umožňuje čtení stavu tlačítek zapojených na odporový
 * dělič (Analog Keypad) připojený k ADC převodníku. Zahrnuje funkce
 * pro inicializaci, raw čtení a čtení s ošetřením zákmitsů (debounce).
 *
 * @author AI Assistant (Gemini)
 * @{
 */

#include <avr/io.h>

/**
 * @brief Identifikátory jednotlivých tlačítek.
 * * Výčet mapuje analogové hodnoty na logická jména tlačítek.
 * BTN_NONE (-1) značí, že žádné tlačítko není stisknuto.
 */
typedef enum {
    BTN_NONE = -1, /**< @brief Žádné tlačítko nestisknuto */
    BTN_S1 = 0,    /**< @brief Tlačítko 1 (vlevo/nejnižší hodnota) */
    BTN_S2 = 1,    /**< @brief Tlačítko 2 */
    BTN_S3 = 2,    /**< @brief Tlačítko 3 */
    BTN_S4 = 3,    /**< @brief Tlačítko 4 */
    BTN_S5 = 4     /**< @brief Tlačítko 5 (vpravo/nejvyšší hodnota) */
} ButtonID;


// --- Deklarace funkcí ---

/**
 * @brief  Inicializuje ADC převodník pro čtení tlačítek.
 * @return none
 * @note   Nastavuje referenci AVCC a prescaler pro frekvenci ADC.
 */
void buttons_init(void);


/**
 * @brief  Přečte okamžitou hodnotu stisknutého tlačítka.
 * @return ID stisknutého tlačítka (ButtonID) nebo BTN_NONE.
 * @note   Tato funkce neobsahuje debounce ani detekci náběžné hrany.
 * Vrací hodnotu na základě aktuálního napětí na ADC pinu.
 */
ButtonID buttons_read_raw(void);


/**
 * @brief  Zjišťuje stisk tlačítka s ošetřením zákmitsů (debounce).
 * @return ID tlačítka pouze při novém stisku, jinak BTN_NONE.
 * @note   Funkce je blokující (obsahuje _delay_ms) v momentě detekce změny.
 * Implementuje logiku "state change detection" - vrátí tlačítko
 * jen jednou při stisku, neopakuje hodnotu při držení.
 */
ButtonID buttons_check_press(void);

/** @} */

#endif /* ADC_BUTTONS_H_ */