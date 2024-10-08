#include "mock_sensors.h"
#include <chess.h>
#include <unity.h>

inline uint64_t EXEC_ONE(Game* p_game, const char*& p_ptr, uint64_t p_sensorsState) {
    TEST_ASSERT_NOT_NULL(p_game);
    TEST_ASSERT_NOT_NULL(p_ptr);
    uint64_t nextSensorsState = updateSensors(p_sensorsState, p_ptr);
    evolveGame(p_game, nextSensorsState);
    return nextSensorsState;
}

// Execute a sequence of piece movements, from direct char*
inline uint64_t EXEC(Game* p_game, const char* p_ptr, uint64_t p_sensorsState) {
    TEST_ASSERT_NOT_NULL(p_ptr);
    uint64_t nextSensorsState = p_sensorsState;

    const char* localPtr = p_ptr;
    while (*localPtr != 0) {
        nextSensorsState = EXEC_ONE(p_game, localPtr, nextSensorsState);
    }

    return nextSensorsState;
}

// Execute a sequence of piece movements, from reference char*
inline uint64_t EXEC_REF(Game* p_game, const char*& p_ptr, uint64_t p_sensorsState) {
    TEST_ASSERT_NOT_NULL(p_ptr);
    uint64_t nextSensorsState = p_sensorsState;

    while (*p_ptr != 0) {
        nextSensorsState = EXEC_ONE(p_game, p_ptr, nextSensorsState);
    }

    return nextSensorsState;
}