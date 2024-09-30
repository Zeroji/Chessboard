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

    char fenBuffer[96];
    writeToFEN(&game, fenBuffer);
    printf("Initial FEN: %s\n", fenBuffer);
    printf("Initial board status:\n");
    printGame(&game);

    // The sensors status are stored in a 64-bits variable: b63 = h8, b62 = g8..., b55 = h7, b54 = g7..., b1 = b1, b0 = a1
    const char* testString = {
        "_"
        "# This is a comment (-d2 +d4 is not parsed)\n"
        "-e2 +e2" // "undo" move
        "-e2 +e4"
        "-d7 +d5"
        "-d5 -e4 +d5" // Capture (captured color removed first)
        "-d8 -d5 +d5" // Capture (playing color removed first)
        "-g1 +f3"
        "-b8 +c6"
        "-f1 +d3"
        "-g8 +f6"
        "-e1 +g1 -h1 +f1" // Castling
        "-f6 +g4"
        "-a2 +a3"
        "-g4 -h2 +h2"
        "-a3 +a4"
        "-h2 +g4"
        "-a4 +a5"
        "-h7 +h5"
        "-a1 +a2" // Stupid rook move to hold white position
        "-h5 +h4"
        "-a2 +a1" // Stupid rook move to hold white position
        "-h4 +h3"
        "-a1 +a2" // Stupid rook move to hold white position
        "-h3 +h2"
        "-a2 +a1" // Stupid rook move to hold white position
        "-h2 +h1" // Promotion and checkmate!
    };

    uint64_t sensorsState = DEFAULT_SENSORS_STATE;
    for (const char* ptr = testString; *ptr != 0;) {
        printf("Checking sensors...\n");
        sensorsState = updateSensors(sensorsState, ptr);

        bool played = evolveGame(&game, sensorsState);
        printf("\tThe state is: %s\n", getStatusStr(game.state.status));
        if (true == played) {
            printGame(&game);
        }
    }

    writeToFEN(&game, fenBuffer);
    printf("Final FEN: %s\n", fenBuffer);

    return 0;
}
