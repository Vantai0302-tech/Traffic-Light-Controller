#include "traffic_light.h"
#include "main.h"

static void traffic_write(GPIO_TypeDef *portA, uint16_t pinA, GPIO_TypeDef *portB, uint16_t pinB, TrafficColor color) {
    // Bit cao so 2
    uint8_t a = ((uint8_t)color >> 1U) & 1U;
    // Bit cao so 1
    uint8_t b = (uint8_t)color & 1U;

    // Set HIGH signals first.
    if (a != 0U) {
        HAL_GPIO_WritePin(portA, pinA, GPIO_PIN_SET);
    }

    if (b != 0U) {
        HAL_GPIO_WritePin(portB, pinB, GPIO_PIN_SET);
    }

    // Then clear LOW signals.
    if (a == 0U) {
        HAL_GPIO_WritePin(portA, pinA, GPIO_PIN_RESET);
    }

    if (b == 0U) {
        HAL_GPIO_WritePin(portB, pinB, GPIO_PIN_RESET);
    }
}

// Dua vao giai ma set mau roi ghi vao io
void traffic_set(uint8_t line, TrafficColor color)
{
    if ((unsigned int)color > (unsigned int)OFF) {
        return;
    }

    switch (line) {
        case 1:
            traffic_write(
                TL1_A_GPIO_Port, TL1_A_Pin,
                TL1_B_GPIO_Port, TL1_B_Pin,
                color
            );
            break;

        case 2:
            traffic_write(
                TL2_A_GPIO_Port, TL2_A_Pin,
                TL2_B_GPIO_Port, TL2_B_Pin,
                color
            );
            break;

        default:
            break;
    }
}

void traffic_init(void)
{
    traffic_all_red();
}

void traffic_all_red(void)
{
    traffic_set(1, RED);
    traffic_set(2, RED);
}


void traffic_all_off(void)
{
    traffic_set(1, OFF);
    traffic_set(2, OFF);
}
