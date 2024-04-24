#pragma once

#include <stdint.h>

extern const uint64_t DEFAULT_SENSORS_STATE;

typedef enum {
    Place = 0,
    Remove,
    Still
} Action;

typedef struct {
    Action action;
    const char* cell;
} SensorUpdate;

uint64_t updateSensors(uint64_t p_state, SensorUpdate p_update);
