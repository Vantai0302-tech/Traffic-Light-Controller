#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <stdint.h>

typedef enum {
    GREEN = 0,
    YELLOW = 1,
    RED    = 2,
    OFF    = 3
} TrafficColor;

void traffic_init(void);
void traffic_set(uint8_t line, TrafficColor color);
void traffic_all_red(void);
void traffic_all_off(void);

#endif