#include "../include/sensors.h"

#include <stdio.h>

const uint64_t DEFAULT_SENSORS_STATE = 0xFFFF00000000FFFFuLL; // (11111111 11111111 00000000 00000000 00000000 00000000 11111111 11111111)

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
        if (command == '+')
            return p_state | ((uint64_t)0x1 << index);
        // else: command == '-'
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
