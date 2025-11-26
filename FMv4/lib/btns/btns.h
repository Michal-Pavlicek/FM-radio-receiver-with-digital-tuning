#ifndef ADC_BUTTONS_H_
#define ADC_BUTTONS_H_

#include <avr/io.h>

// Definice návratových hodnot (abychom v kódu nepsali čísla 0,1,2...)
typedef enum {
    BTN_NONE = -1,
    BTN_S1 = 0,
    BTN_S2 = 1,
    BTN_S3 = 2,
    BTN_S4 = 3,
    BTN_S5 = 4
} ButtonID;

// --- Deklarace funkcí (to, co může volat programátor) ---

// Inicializuje ADC převodník
void buttons_init(void);

// Zjistí, které tlačítko je právě zmáčknuté (vrací raw stav bez debouncingu)
ButtonID buttons_read_raw(void);

// Čte tlačítka a rovnou řeší debounce a změnu stavu (blokující čekání)
// Vrací ID tlačítka jen při novém stisku, jinak vrací BTN_NONE
ButtonID buttons_check_press(void);

#endif /* ADC_BUTTONS_H_ */