#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

#define BUTTON_COUNT          4
#define BUTTON_DEBOUNCE_TICKS 3
#define BUTTON_LONG_TICKS     100

void button_init(void);
void button_run(void);

uint8_t isButtonPressed(uint8_t id);
uint8_t isButtonLongPressed(uint8_t id);
uint8_t isButtonDown(uint8_t id);

#endif