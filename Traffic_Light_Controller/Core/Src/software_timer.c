#include "software_timer.h"

static uint32_t timer_counter[TIMER_COUNT];
uint8_t timer_flag[TIMER_COUNT];

void timer_init(void) {
    for (uint8_t i = 0; i < TIMER_COUNT; i++ ) {
        timer_counter[i] = 0;
        timer_flag[i] = 0;
    }
}

void setTimer(uint8_t id, uint32_t duration_ms) {
    if (id >= TIMER_COUNT) return;

    if (duration_ms == 0) {
        timerStop(id);
        return;
    }

    timer_counter[id] = duration_ms / TIMER_TICKS_MS;

    timer_flag[id] = 0;
}

void timerRun(void) {
    for (int i = 0; i < TIMER_COUNT; i++) {
        if (timer_counter[i] > 0) {
            timer_counter[i]--;
        }

        if (timer_counter[i] == 0U) {
            timer_flag[i] = 1U;
        }
    }
}

void timerStop(uint8_t id)
{
    if (id >= TIMER_COUNT) return;

    timer_counter[id] = 0U;
    timer_flag[id] = 0U;
}