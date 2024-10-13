#include "utils.h"
#include <chess.h>
#include <unity.h>

static void test_getMoveStr() {
    Game game;
    initializeFromFEN(&game, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    uint64_t sensors = DEFAULT_SENSORS_STATE;

    char moves[][16] = {
        // "sensors action", "string reaction",
        "-e2 +e4",
        "e4",
        "-d7 +d5",
        "d5",
        "-e4 -d5 +d5",
        "exd5",
        "-d8 -d5 +d5",
        "Qxd5",
        "-f1 +b5",
        "Bb5+",
        "-c8 +d7",
        "Bd7",
        "-g1 +f3",
        "Nf3",
        "-b8 +c6",
        "Nc6",
        "-e1 +g1 -h1 +f1",
        "O-O",
        "-e8 +c8 -a8 +d8",
        "O-O-O",
        "-h2 +h4",
        "h4",
        "-c8 +b8",
        "Kb8",
        "-h4 +h5",
        "h5",
        "-g7 +g5",
        "g5",
        "-h5 +g6 -g5",
        "hxg6", // en passant
        "-d7 +h3",
        "Bh3",
        "-f3 +e5",
        "Ne5",
        "-d5 -g2 +g2",
        "Qxg2#",
    };

    Move* lastMovePtr;
    for (uint8_t i = 0; i < (sizeof(moves) / sizeof(moves[0])) / 2; i++) {
        sensors     = EXEC(&game, moves[2 * i], sensors);
        lastMovePtr = (i % 2 == 0) ? &game.lastMoveW : &game.lastMoveB;
        TEST_ASSERT_EQUAL_STRING(moves[2 * i + 1], getMoveStr(*lastMovePtr));
    }
}

void run_moves() {
    UNITY_BEGIN();

    RUN_TEST(test_getMoveStr);

    UNITY_END();
}