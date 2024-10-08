#pragma once

#include <chess.h>
#include <stdint.h>


uint64_t updateSensors(uint64_t p_state, const char*& p_actionPtr);
uint64_t extractSensorsState(Game* p_game);
