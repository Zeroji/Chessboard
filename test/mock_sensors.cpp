#include "mock_sensors.h"

#include <stdio.h>

//-----------------------------------------------------------------------------
uint64_t updateSensors(uint64_t p_state, const char*& p_actionPtr)
//-----------------------------------------------------------------------------
{
    const char command = *p_actionPtr++;

    switch (command) {
    case 0:
    case '_':
        return p_state;
    case '+':
    case '-': {
        char letter = *(p_actionPtr);
        char number = *(p_actionPtr+1);
        uint8_t col = ((uint8_t)(*p_actionPtr++) & 0b11011111) - (uint8_t)('A'); // clear lowercase bit then move to 0-7
        if (col >= 8) {
            printf("Unable to update sensors with invalid cell col: %c\n", *(p_actionPtr - 1));
            return p_state;
        }

        uint8_t row = (uint8_t)(*p_actionPtr++) - (uint8_t)('1');
        if (row >= 8) {
            printf("Unable to update sensors with invalid cell row: %c\n", *(p_actionPtr - 1));
            return p_state;
        }

        uint8_t index = col + 8 * row;

        if (command == '+') {
            printf("+%c%c\n", letter, number);
            return p_state | ((uint64_t)0x1 << index);
        }
        // else: command == '-'
        printf("-%c%c\n", letter, number);
        return p_state & ~((uint64_t)0x1 << index);
    }
    case '#':
        // Skip all characters until \0 (return) or \n (recurse)
        while (*p_actionPtr++ != '\n')
            if (*p_actionPtr == 0)
                return p_state;
        [[fallthrough]];
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        return updateSensors(p_state, p_actionPtr);
    default:
        printf("Unable to update sensors with invalid action: %c\n", command);
        return p_state;
    }
}

//-----------------------------------------------------------------------------
uint64_t extractSensorsState(Game* p_game)
//-----------------------------------------------------------------------------
{
    if (nullptr == p_game) {
        printf("Unable to extract sensors state from null game\n");
        return 0;
    }

    // EPiece board[64]; // a1, b1, c1..., a2, b2, c2...
    // b63 = h8, b62 = g8..., b55 = h7, b54 = g7..., b1 = b1, b0 = a1
    uint64_t state = 0;
    for (uint8_t index = 0; index < 64; index++) {
        if (EPiece::Empty != p_game->board[index]) {
            state |= (1uLL << index);
        }
    }

    return state;
}