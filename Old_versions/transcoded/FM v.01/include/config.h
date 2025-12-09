#ifndef CONFIG_H
#define CONFIG_H

#include <avr/io.h>

/* --- Rádio Si4703 --- */
#define SI4703_RESET_DDR   (&DDRB)
#define SI4703_RESET_PORT  (&PORTB)
#define SI4703_RESET_PIN   PB0

#define SI4703_SDIO_DDR    (&DDRC)
#define SI4703_SDIO_PORT   (&PORTC)
#define SI4703_SDIO_PIN    PC4

#define SI4703_SCLK_DDR    (&DDRC)
#define SI4703_SCLK_PORT   (&PORTC)
#define SI4703_SCLK_PIN    PC5

/* --- UART --- */
#define UART_BAUDRATE 9600

#endif