#include "traffic_light.h"
#include "main.h"

static void traffic_write(GPIO_TypeDef *portA, uint16_t pinA, GPIO_TypeDef *portB, uint16_t pinB, TrafficColor color) {
    uint8_t hardware_code;

    switch (color) {
    case GREEN:  
        hardware_code = 2U; 
        break; // GPIO 10
    case YELLOW: 
        hardware_code = 1U; 
        break; // GPIO 01
    case RED:    
        hardware_code = 3U; 
        break; // GPIO 11
    case OFF:    
        hardware_code = 0U; 
        break; // GPIO 00
    default: return;
}

uint8_t a = (hardware_code >> 1U) & 1U;
uint8_t b = hardware_code & 1U;

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
