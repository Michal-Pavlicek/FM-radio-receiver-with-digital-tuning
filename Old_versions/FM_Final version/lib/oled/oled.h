/*
 * This file is part of lcd library for ssd1306/ssd1309/sh1106 oled-display.
 *
 * lcd library for ssd1306/ssd1309/sh1106 oled-display is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or any later version.
 *
 * lcd library for ssd1306/ssd1309/sh1106 oled-display is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Diese Datei ist Teil von lcd library for ssd1306/ssd1309/sh1106 oled-display.
 *
 * lcd library for ssd1306/ssd1309/sh1106 oled-display ist Freie Software: Sie können es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder jeder späteren
 * veröffentlichten Version, weiterverbreiten und/oder modifizieren.
 *
 * lcd library for ssd1306/ssd1309/sh1106 oled-display wird in der Hoffnung, dass es nützlich sein wird, aber
 * OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
 * Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License für weitere Details.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.
 *
 *  lcd.h
 *
 *  Created by Michael Köhler on 22.12.16.
 *  Copyright 2016 Skie-Systems. All rights reserved.
 *
 *  lib for OLED-Display with ssd1306/ssd1309/sh1106-Controller
 *  first dev-version only for I2C-Connection
 *  at ATMega328P like Arduino Uno
 *
 *  at GRAPHICMODE lib needs SRAM for display
 *  DISPLAY-WIDTH * DISPLAY-HEIGHT + 2 bytes
 */

#ifndef OLED_H
#define OLED_H

/**
 * @file oled.h
 * @defgroup oled_lib OLED Display Library <oled.h>
 * @code #include "oled.h" @endcode
 *
 * @brief Knihovna pro grafické OLED displeje (SSD1306/SH1106).
 *
 * Tato knihovna poskytuje funkce pro ovládání monochromatických OLED displejů
 * připojených přes I2C nebo SPI. Podporuje textový i grafický režim.
 * V grafickém režimu využívá "frame buffer" v RAM mikrokontroléru.
 *
 * @author Michael Köhler (Skie-Systems) - Původní autor kódu
 * @author AI Assistant (Gemini) - Generování Doxygen dokumentace
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif
    
#if (__GNUC__ * 100 + __GNUC_MINOR__) < 303
# error "This library requires AVR-GCC 3.3 or later, update to newer AVR-GCC compiler !"
#endif

#include <inttypes.h>
#include <avr/pgmspace.h>

/* -- Konfigurace -- */

/** @brief Definice komunikační sběrnice (I2C nebo SPI) */
#define I2C  
// #define SPI

/** @brief Typ kontroléru displeje (SH1106 nebo SSD1306) */
#define SH1106  
// #define SSD1306

/** @brief Režim displeje (GRAPHICMODE = text i grafika, TEXTMODE = jen text) */
#define GRAPHICMODE  
// #define TEXTMODE

/** @brief Použitý font (definovaný ve font.h) */
#define FONT  ssd1306oled_font  
    
/** @brief I2C adresa displeje (7-bit, bez R/W bitu) */
#define OLED_I2C_ADR (0x3c) 


#ifdef I2C
# include "twi.h"
#elif defined SPI
# define OLED_PORT PORTB
# define OLED_DDR  DDRB
# define RES_PIN  PB0
# define DC_PIN   PB1
# define CS_PIN   PB2
#endif

#ifndef YES
# define YES 1
#endif

#define NORMALSIZE 1 /**< @brief Normální velikost písma (5x7 px) */
#define DOUBLESIZE 2 /**< @brief Dvojnásobná velikost písma */
    
#define OLED_DISP_OFF 0xAE /**< @brief Příkaz pro vypnutí displeje */
#define OLED_DISP_ON 0xAF  /**< @brief Příkaz pro zapnutí displeje */
    
#define WHITE 0x01 /**< @brief Barva pixelu: Svítí */
#define BLACK 0x00 /**< @brief Barva pixelu: Nesvítí */
    
#define DISPLAY_WIDTH 128 /**< @brief Šířka displeje v pixelech */
#define DISPLAY_HEIGHT 64 /**< @brief Výška displeje v pixelech */

// -- Funkční prototypy --

/**
 * @brief  Odešle příkaz do řadiče displeje.
 * @param  cmd Pole bajtů s příkazy.
 * @param  size Počet bajtů k odeslání.
 */
void oled_command(uint8_t cmd[], uint8_t size);


/**
 * @brief  Odešle data (obrazová data) do řadiče displeje.
 * @param  data Pole bajtů s daty.
 * @param  size Počet bajtů k odeslání.
 */
void oled_data(uint8_t data[], uint16_t size);


/**
 * @brief  Inicializuje OLED displej.
 * @param  dispAttr Atributy displeje (obvykle OLED_DISP_ON/OFF).
 * @note   Nastaví hardwarové piny, inicializuje I2C/SPI a odešle
 * inicializační sekvenci příkazů do řadiče displeje.
 */
void oled_init(uint8_t dispAttr);


/**
 * @brief  Nastaví kurzor na pozici 0,0 (levý horní roh).
 */
void oled_home(void);


/**
 * @brief  Invertuje zobrazení na displeji (negativ).
 * @param  invert YES (1) pro inverzi, jinak normální zobrazení.
 */
void oled_invert(uint8_t invert);


/**
 * @brief  Přepne displej do režimu spánku (vypne obrazovku).
 * @param  sleep YES (1) pro spánek, jinak probuzení.
 */
void oled_sleep(uint8_t sleep);


/**
 * @brief  Nastaví kontrast displeje.
 * @param  contrast Hodnota kontrastu (0-255).
 */
void oled_set_contrast(uint8_t contrast);


/**
 * @brief  Vypíše řetězec textu na displej.
 * @param  s Ukazatel na řetězec v RAM (ukončený null znakem).
 * @note   V GRAPHICMODE píše do bufferu, v TEXTMODE přímo na displej.
 */
void oled_puts(const char* s);


/**
 * @brief  Vypíše řetězec textu z programové paměti (PROGMEM).
 * @param  progmem_s Ukazatel na řetězec ve flash paměti.
 */
void oled_puts_p(const char* progmem_s);


/**
 * @brief  Vymaže obsah obrazovky (a bufferu v GRAPHICMODE).
 */
void oled_clrscr(void);


/**
 * @brief  Nastaví pozici kurzoru pro text.
 * @param  x Sloupec (ve znacích).
 * @param  y Řádek (stránka, 0-7).
 */
void oled_gotoxy(uint8_t x, uint8_t y);


/**
 * @brief  Nastaví pozici kurzoru s přesností na pixely (osa X).
 * @param  x Sloupec (v pixelech).
 * @param  y Řádek (stránka, 0-7).
 */
void oled_goto_xpix_y(uint8_t x, uint8_t y);


/**
 * @brief  Vypíše jeden znak.
 * @param  c Znak k vypsání.
 */
void oled_putc(char c);


/**
 * @brief  Nastaví velikost písma.
 * @param  mode NORMALSIZE (1) nebo DOUBLESIZE (2).
 */
void oled_charMode(uint8_t mode);


/**
 * @brief  Otočí zobrazení displeje (flip).
 * @param  flipping Mód otočení:
 * - 0: Normální
 * - 1: Otočení o 180° (horizontálně i vertikálně)
 * - 2: Zrcadlení vertikálně
 * - 3: Zrcadlení horizontálně
 */
void oled_flip(uint8_t flipping);

#if defined GRAPHICMODE
    /**
     * @brief  Vykreslí bod na souřadnicích X, Y.
     * @param  x Souřadnice X (0-127).
     * @param  y Souřadnice Y (0-63).
     * @param  color Barva (WHITE / BLACK).
     * @return 0 při úspěchu, 1 mimo rozsah.
     */
    uint8_t oled_drawPixel(uint8_t x, uint8_t y, uint8_t color);

    /**
     * @brief  Vykreslí úsečku mezi dvěma body (Bresenhamův algoritmus).
     */
    uint8_t oled_drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

    /**
     * @brief  Vykreslí obrys obdélníku.
     */
    uint8_t oled_drawRect(uint8_t px1, uint8_t py1, uint8_t px2, uint8_t py2, uint8_t color);

    /**
     * @brief  Vykreslí vyplněný obdélník.
     */
    uint8_t oled_fillRect(uint8_t px1, uint8_t py1, uint8_t px2, uint8_t py2, uint8_t color);

    /**
     * @brief  Vykreslí kružnici.
     */
    uint8_t oled_drawCircle(uint8_t center_x, uint8_t center_y, uint8_t radius, uint8_t color);

    /**
     * @brief  Vykreslí vyplněný kruh.
     */
    uint8_t oled_fillCircle(uint8_t center_x, uint8_t center_y, uint8_t radius, uint8_t color);

    /**
     * @brief  Vykreslí bitmapu (obrázek) z pole dat.
     */
    uint8_t oled_drawBitmap(uint8_t x, uint8_t y, const uint8_t picture[], uint8_t width, uint8_t height, uint8_t color);

    /**
     * @brief  Odešle obsah bufferu v RAM na displej.
     * @note   Tuto funkci je nutné volat po kreslení v GRAPHICMODE, 
     * jinak se změny neprojeví.
     */
    void oled_display(void);

    /**
     * @brief  Vymaže obsah bufferu v RAM (nastaví na 0).
     */
    void oled_clear_buffer(void);

    /**
     * @brief  Zjistí stav pixelu v bufferu.
     * @return Barva pixelu na dané souřadnici.
     */
    uint8_t oled_check_buffer(uint8_t x, uint8_t y);

    /**
     * @brief  Odešle pouze část bufferu (jeden řádek/page) na displej.
     */
    void oled_display_block(uint8_t x, uint8_t line, uint8_t width);
#endif

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* OLED_H  */