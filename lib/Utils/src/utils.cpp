#include "utils.h"

uint32_t timeDiff(uint32_t from_ms, uint32_t to_ms) {
    uint32_t diff_ms = 0;

    if (to_ms < from_ms) {
        // Handle timestamp overflow to 0
        uint32_t shift = 0xFFFFFFFF - from_ms;
        diff_ms        = shift + to_ms;
    } else {
        diff_ms = to_ms - from_ms;
    }

    return diff_ms;
}

uint64_t stabilizeValue(uint64_t p_boardState, uint32_t p_now_ms, uint32_t p_delay) {
    static uint64_t lastStableBoard    = p_boardState;
    static uint32_t lastStableBoard_ms = p_now_ms;

    static uint64_t lastBoard    = p_boardState;
    static uint32_t lastBoard_ms = p_now_ms;

    if (p_boardState != lastBoard) {
        // New board state received
        if (timeDiff(lastBoard_ms, p_now_ms) >= p_delay) {
            // New board state received more than DELAY since previous one: update stable board
            lastBoard          = p_boardState;
            lastBoard_ms       = p_now_ms;
            lastStableBoard    = p_boardState;
            lastStableBoard_ms = p_now_ms;
            return lastStableBoard;
        }

        // New board state received less than DELAY since previous one: return stable board
        lastBoard    = p_boardState;
        lastBoard_ms = p_now_ms;

        return lastStableBoard;
    }

    // Same board state received
    if (timeDiff(lastStableBoard_ms, p_now_ms) >= p_delay) {
        // Same board state received more than DELAY since previous stable board: update stable board
        lastBoard          = p_boardState;
        lastBoard_ms       = p_now_ms;
        lastStableBoard    = p_boardState;
        lastStableBoard_ms = p_now_ms;
        return lastStableBoard;
    }

    // Same board state received less than DELAY since previous stable board: return stable board
    lastBoard    = p_boardState;
    lastBoard_ms = p_now_ms;
    return lastStableBoard;
}