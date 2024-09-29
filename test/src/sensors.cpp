#include "../include/sensors.h"

#include <stdio.h>

const uint64_t DEFAULT_SENSORS_STATE = 18446462598732906495U; // (1111111111111111000000000000000000000000000000001111111111111111)

//-----------------------------------------------------------------------------
uint64_t updateSensors(uint64_t p_state, SensorUpdate p_update)
//-----------------------------------------------------------------------------
{
    if (Still == p_update.action) {
        return p_state;
    }

    if (NULL == p_update.cell) {
        printf("Unable to update sensors with null cell\n");
        return p_state;
    }

    uint8_t col = (uint8_t)(p_update.cell[0]) - (uint8_t)('a');
    if (col >= 8) {
        printf("Unable to update sensors with invalid cell col: %c\n", p_update.cell[0]);
        return p_state;
    }

    uint8_t row = (uint8_t)(p_update.cell[1]) - (uint8_t)('1');
    if (row >= 8) {
        printf("Unable to update sensors with invalid cell row: %c\n", p_update.cell[1]);
        return p_state;
    }

    uint8_t index = col + 8 * row;

    if (Place == p_update.action) {
        return p_state | ((uint64_t)0x1 << index);
    } else if (Remove == p_update.action) {
        return p_state & ~((uint64_t)0x1 << index);
    }

    printf("Unable to update sensors with invalid action: %d\n", p_update.action);
    return p_state;
}
