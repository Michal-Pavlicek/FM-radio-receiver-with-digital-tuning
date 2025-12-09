#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "oled.h"
#include "si4703_breakout.h"
#include "gpio.h"
#include "twi.h"
#include "uart.h"
#include "config.h"

int frekvence;
int hlasitost;
char rdsBuffer[9];

void zobrazInfo(void) {
    char line[32];
    sprintf(line, "FM %d.%d MHz | Vol:%d", frekvence/10, frekvence%10, hlasitost);
    uart_puts(line);
    uart_puts("\r\n");
}

int main(void) {
    // Inicializace UARTu na 9600 baud
    uart_init(UART_BAUD_SELECT(9600, F_CPU));

    oled_init(0);
    oled_clrscr();
    oled_puts("Si4703 Radio");

    // Použití #define pinů z config.h
    Si4703_HW hw = {
        .reset_ddr  = SI4703_RESET_DDR,
        .reset_port = SI4703_RESET_PORT,
        .reset_pin  = SI4703_RESET_PIN,
        .sdio_ddr   = SI4703_SDIO_DDR,
        .sdio_port  = SI4703_SDIO_PORT,
        .sdio_pin   = SI4703_SDIO_PIN,
    };

    si4703_init(&hw);
    hlasitost = 0;
    si4703_setVolume(hlasitost);

    while (1) {
        unsigned int data = uart_getc();
        if (!(data & UART_NO_DATA)) {
            char ch = (char)data;

            if (ch == 'n') {
                frekvence = si4703_seekUp();
                zobrazInfo();
            }
            else if (ch == 'd') {
                frekvence = si4703_seekDown();
                zobrazInfo();
            }
            else if (ch == '+') {
                if (hlasitost < 15) hlasitost++;
                si4703_setVolume(hlasitost);
                zobrazInfo();
            }
            else if (ch == '-') {
                if (hlasitost > 0) hlasitost--;
                si4703_setVolume(hlasitost);
                zobrazInfo();
            }
            else if (ch == 'a') {
                frekvence = 910; // Radio Beat
                si4703_setChannel(frekvence);
                zobrazInfo();
            }
            else if (ch == 'b') {
                frekvence = 1055; // Evropa 2
                si4703_setChannel(frekvence);
                zobrazInfo();
            }
            else if (ch == 'r') {
                uart_puts("Nacteni RDS dat...\r\n");
                si4703_readRDS(rdsBuffer, 15000);
                uart_puts("RDS data: ");
                uart_puts(rdsBuffer);
                uart_puts("\r\n");
            }
        }
    }

    return 0;
}