#pragma once

#include <stdint.h>

extern const uint64_t DEFAULT_SENSORS_STATE;

uint64_t updateSensors(uint64_t p_state, const char*& p_actionPtr);
