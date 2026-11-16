#include "fsm_automatic.h"
#include "software_timer.h"
#include "traffic_light.h"

#define AUTO_TIMER_ID       0U

#define TIME_GREEN1_MS     6000U
#define TIME_YELLOW1_MS    2000U
#define TIME_GREEN2_MS     4000U
#define TIME_YELLOW2_MS    2000U

typedef enum {
    AUTO_P0,
    AUTO_P1,
    AUTO_P2,
    AUTO_P3
} AutoState;

static AutoState auto_state;
static uint8_t auto_active = 0U;

static void fsm_automatic_enter(AutoState next) {
    auto_state = next;
    // Set full do roi moi set tung den theo state
    traffic_all_red();

    switch (auto_state) {
        case AUTO_P0:
            traffic_set(2, GREEN);
            setTimer(AUTO_TIMER_ID, TIME_GREEN2_MS);
            break;

        case AUTO_P1:
            traffic_set(2, YELLOW);
            setTimer(AUTO_TIMER_ID, TIME_YELLOW2_MS);
            break;

        case AUTO_P2:
            traffic_set(1, GREEN);
            setTimer(AUTO_TIMER_ID, TIME_GREEN1_MS);
            break;

        case AUTO_P3:
            traffic_set(1, YELLOW);
            setTimer(AUTO_TIMER_ID, TIME_YELLOW1_MS);
            break;

        default:
            break;
    }
}

void fsm_automatic_init(void)
{
    auto_active = 1U;
    fsm_automatic_enter(AUTO_P0);
}

void fsm_automatic_run(void) {
    if (auto_active == 0U) {
        return;
    }

    if (timer_flag[AUTO_TIMER_ID] == 0U) {
        return;
    }

    switch (auto_state) {
        case AUTO_P0:
            fsm_automatic_enter(AUTO_P1);
            break;

        case AUTO_P1:
            fsm_automatic_enter(AUTO_P2);
            break;

        case AUTO_P2:
            fsm_automatic_enter(AUTO_P3);
            break;

        case AUTO_P3:
            fsm_automatic_enter(AUTO_P0);
            break;

        default:
            fsm_automatic_enter(AUTO_P0);
            break;
    }
}

void fsm_automatic_stop(void)
{
    auto_active = 0U;
    timerStop(AUTO_TIMER_ID);
    traffic_all_red();
}