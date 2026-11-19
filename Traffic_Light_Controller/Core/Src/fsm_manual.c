
#include "fsm_manual.h"
#include "button.h"
#include "traffic_light.h"

#define BTN_NEXT_ID  1U

typedef enum {
    MAN_P0,
    MAN_P1,
    MAN_P2,
    MAN_P3
} ManualState;

static ManualState manual_state;
static uint8_t manual_active = 0U;


static void fsm_manual_enter(ManualState next)
{
    manual_state = next;

    traffic_all_red();

    switch (manual_state) {
        case MAN_P0:
            traffic_set(2, GREEN);
            break;

        case MAN_P1:
            traffic_set(2, YELLOW);
            break;

        case MAN_P2:
            traffic_set(1, GREEN);
            break;

        case MAN_P3:
            traffic_set(1, YELLOW);
            break;

        default:
            break;
    }
}

void fsm_manual_init(void)
{
    manual_active = 1U;
    fsm_manual_enter(MAN_P0);
}


void fsm_manual_run(void)
{
    if (manual_active == 0U) {
        return;
    }

    if (isButtonPressed(BTN_NEXT_ID) == 0U) {
        return;
    }

    switch (manual_state) {
        case MAN_P0:
            fsm_manual_enter(MAN_P1);
            break;

        case MAN_P1:
            fsm_manual_enter(MAN_P2);
            break;

        case MAN_P2:
            fsm_manual_enter(MAN_P3);
            break;

        case MAN_P3:
            fsm_manual_enter(MAN_P0);
            break;

        default:
            fsm_manual_enter(MAN_P0);
            break;
    }
}

void fsm_manual_stop(void)
{
    manual_active = 0U;
    traffic_all_red();
}
