#include "button.h"
#include "main.h"

#define BUTTON_ACTIVE_LEVEL GPIO_PIN_RESET

typedef struct {
    uint8_t last_sample;
    // Trạng thái đã qua lọc nhiễu
    uint8_t stable_pressed;
    // Biến đếm bao nhiêu lần liên tiếp
    uint8_t sample_count;
    // Cờ báo sự khi có nút nhất khi stable_press từ 0 lên 1
    uint8_t press_event;
    // Bật lên 1 lúc hold_ticks chạm ngưỡng và long_sent = 0
    uint8_t long_event;

    // Số tick đã giữ 
    uint16_t hold_ticks;
    // Cờ để xác nhận chỉ bấm 1 lần long press
    uint8_t long_sent;
} ButtonState;

static ButtonState buttons[BUTTON_COUNT];
// Config GPIO Port and Pin
static GPIO_TypeDef * const button_ports[BUTTON_COUNT] = {
    BTN_1_GPIO_Port,
    BTN_2_GPIO_Port,
    BTN_3_GPIO_Port,
    BTN_4_GPIO_Port,
    B1_GPIO_Port    // nút USER (PC13), id 4
};

static const uint16_t button_pins[BUTTON_COUNT] = {
    BTN_1_Pin,
    BTN_2_Pin,
    BTN_3_Pin,
    BTN_4_Pin,
    B1_Pin          // nút USER (PC13), id 4
};

static uint8_t button_read_raw(uint8_t id) {
    GPIO_PinState state = HAL_GPIO_ReadPin(
        button_ports[id],
        button_pins[id]
    );

    if (state == BUTTON_ACTIVE_LEVEL) {
        return 1;
    } else {
        return 0;
    }
}

void button_init(void)
{
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        uint8_t raw = button_read_raw(i);

        buttons[i].last_sample = raw;
        buttons[i].stable_pressed = raw;
        buttons[i].sample_count = 0U;

        buttons[i].press_event = 0U;
        buttons[i].long_event = 0U;

        buttons[i].hold_ticks = 0U;
        buttons[i].long_sent = 0U;
    }
}

void button_run(void) {
    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        ButtonState *btn = &buttons[i];
        uint8_t raw = button_read_raw(i);

        if (raw == btn->last_sample) {
            // bam roi thi tang count
            if (btn->sample_count < BUTTON_DEBOUNCE_TICKS) {
                btn->sample_count++;
            }
        } else {
            // Chua bam thi set = 1
            btn->last_sample = raw;
            btn->sample_count = 1U;
        }

        if (btn->sample_count >= BUTTON_DEBOUNCE_TICKS &&
            raw != btn->stable_pressed) {
                
            btn->stable_pressed = raw;
            
            if (raw == 1U) {
                btn->press_event = 1U;
                btn->hold_ticks = 0U;
                btn->long_sent = 0U;
            } else {
                btn->hold_ticks = 0U;
                btn->long_sent = 0U;
            }
        }

        if (btn->stable_pressed == 1U) {
            if (btn->hold_ticks < BUTTON_LONG_TICKS) {
                btn->hold_ticks++;
            }

            if (btn->hold_ticks >= BUTTON_LONG_TICKS &&
                btn->long_sent == 0U) {
                btn->long_event = 1U;
                btn->long_sent = 1U;
            }
        }
    }
}

uint8_t isButtonPressed(uint8_t id) {
    if (id >= BUTTON_COUNT) return 0;

    uint8_t event = buttons[id].press_event;
    buttons[id].press_event = 0U;

    return event;
}


uint8_t isButtonLongPressed(uint8_t id)
{
    if (id >= BUTTON_COUNT) return 0U;

    uint8_t event = buttons[id].long_event;
    buttons[id].long_event = 0U;

    return event;
}


uint8_t isButtonDown(uint8_t id)
{
    if (id >= BUTTON_COUNT) return 0U;

    return buttons[id].stable_pressed;
}