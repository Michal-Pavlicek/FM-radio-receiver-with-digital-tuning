#include <avr/io.h>
#include "si4703_breakout.h"
#include "oled.h"

int main(void) {
    // OLED
    oled_init(0);
    oled_clrscr();
    oled_puts("Radio init...");

    // HW konfigurace rádia (přizpůsob svému zapojení)
    Si4703_HW hw = {
        .reset_ddr  = &DDRB,
        .reset_port = &PORTB,
        .reset_pin  = PB0,
        .sdio_ddr   = &DDRC,
        .sdio_port  = &PORTC,
        .sdio_pin   = PC4,  // SDA pin
    };

    si4703_init(&hw);

    // Naladit 101.7 MHz
    si4703_setChannel(1017);

    // Zobrazit frekvenci
    int f = si4703_getChannel();
    char line[20];
    sprintf(line, "FM %d.%d MHz", f/10, f%10);
    oled_clrscr();
    oled_puts(line);

    // RDS PS name
    char ps[9];
    si4703_readRDS(ps, 1000);
    if (ps[0] != '\0') {
        oled_gotoxy(0, 1);
        oled_puts(ps);
    }

    while (1) { }
    return 0;
}
