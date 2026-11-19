#ifndef SOFTWARE_TIMER_H
#define SOFTWARE_TIMER_H

#include <stdint.h>

#define TIMER_COUNT    4
#define TIMER_TICKS_MS 10

extern uint8_t timer_flag[TIMER_COUNT];

void timer_init(void);
void setTimer(uint8_t id, uint32_t duration_ms);
void timerRun(void);
void timerStop(uint8_t id);

#endif 