#include "../../include/chess.h"
#include "../include/sensors.h"

#include <stdint.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
int main()
//-----------------------------------------------------------------------------
{
    Game game;
    initializeGame(&game, DEFAULT_SENSORS_STATE);
    printf("Game initialized\n");

    printf("Initial board status:\n");
    printGame(&game);

    // The sensors status are stored in a 64-bits variable: b63 = h8, b62 = g8..., b55 = h7, b54 = g7..., b1 = b1, b0 = a1
    SensorUpdate sensors[25] = {
        {Still,  "__"},
        {Remove, "e2"},
        {Place,  "e2"}, // "undo" move
        {Remove, "e2"},
        {Place,  "e4"},
        {Remove, "d7"},
        {Place,  "d5"},
        {Remove, "d5"},
        {Remove, "e4"},
        {Place,  "d5"}, // Capture (captured color removed first)
        {Remove, "d8"},
        {Remove, "d5"},
        {Place,  "d5"}, // Capture (playing color removed first)
        {Remove, "g1"},
        {Place,  "f3"},
        {Remove, "b8"},
        {Place,  "c6"},
        {Remove, "f1"},
        {Place,  "d3"},
        {Remove, "g8"},
        {Place,  "f6"},
        {Remove, "e1"}, // Castling (start)
        {Place,  "g1"},
        {Remove, "h1"},
        {Place,  "f1"}, // Castling (end)
    };

    uint64_t sensorsState = DEFAULT_SENSORS_STATE;
    for (uint8_t i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
        printf("Checking sensors...\n");
        sensorsState = updateSensors(sensorsState, sensors[i]);

        bool played = evolveGame(&game, sensorsState);
        printf("\tThe state is: %s\n", getStatusStr(game.state.status));
        if (true == played) {
            printGame(&game);
        }
    }

    return 0;
}