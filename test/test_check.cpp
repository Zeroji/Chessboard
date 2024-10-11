#include "utils.h"
#include <chess.h>
#include <stdio.h>
#include <unity.h>

static void test_check() {
    char fens[][90] = {
        "r6r/1b2k1bq/8/8/7B/8/8/R3K2R b KQ - 3 2",                                  // Bishop
        "8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3",                                       // Pawn
        "r1b1kbnr/pp1ppppp/n7/q1p5/8/P2P1N2/1PP1PPPP/RNBQKB1R w KQkq - 2 2",        // Queen
        "r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQkq - 3 2",       // Queen
        "2kr3r/p1ppqpb1/bN2Qnp1/3P4/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 3 2",           // Knight
        "rnb2k2/pp1Pbppp/2p5/q7/2B5/6n1/PPPQN1PP/RNB1K2r w Q - 3 9",                // Rook
        "2r5/3pk3/1P6/8/8/2K5/8/8 w - - 5 4",                                       // Rook
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1Q1PP/R4RK1 w - - 0 10", // Bishop
        "3k4/3pPK2/8/7r/8/8/8/8 b - - 0 1",                                         // Pawn
        "r3k2r/1b5q/8/8/8/2b5/7B/R3K2R w KQkq - 0 1",                               // Bishop
        "8/8/2k5/5q2/8/3n4/5K2/8 w - - 0 1",                                        // Knight + Queen
        "8/8/8/8/4k1B1/8/1r6/r5K1 w - - 0 1",                                       // Intercepting check
        "1q5k/8/8/Ppnn4/1nKn4/1nnn4/8/8 w - b6 0 1",                                // En-passant saving checkmate
        "1q5k/8/8/1pPn4/1nKn4/1nnn4/8/8 w - b6 0 1",                                // En-passant saving checkmate
        "8/8/8/8/8/2K5/8/1k5R b - - 0 1",                                           // King can escape
    };

    for (uint8_t i = 0; i < sizeof(fens) / sizeof(fens[0]); i++) {
        Game game;
        initializeFromFEN(&game, fens[i]);
        TEST_ASSERT_TRUE_MESSAGE(isCheck(&game), fens[i]);
        TEST_ASSERT_FALSE_MESSAGE(isCheckmate(&game), fens[i]);
    }
}

static void test_notCheck() {
    char fens[][90] = {
        "r6r/1b1k2bq/8/8/7B/8/8/R3K2R b KQ - 3 2",
        "8/8/8/2kP4/2p5/8/B7/4K3 b - - 0 3",
        "r1b1kbnr/pp1ppppp/n7/q1p5/8/P1NP1N2/1PP1PPPP/R1BQKB1R w KQkq - 2 2",
        "r2k3r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 3 2",
        "2kr3r/p1ppqpb1/b1N1Qnp1/3P4/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 3 2",
        "rnb2k2/pp1Pbppp/2p5/q7/2B5/6n1/PPPQNKPP/RNB4r w - - 3 9",
        "2r5/3pk1K1/1P6/8/8/8/8/8 w - - 5 4",
        "r4rk1/1pp1qpp1/p1np1n2/2b1p1B1/2B1P1b1/P1NP1NKp/1PP1Q1PP/R4R2 w - - 0 10",
        "2k5/3pPK2/8/7r/8/8/8/8 b - - 0 1",
        "r3k2r/1b5q/8/8/8/2r5/7B/R3K2R w KQkq - 0 1",
        "8/8/2k5/5q2/8/3n4/4KR2/8 w - - 0 1",
    };

    for (uint8_t i = 0; i < sizeof(fens) / sizeof(fens[0]); i++) {
        Game game;
        initializeFromFEN(&game, fens[i]);
        TEST_ASSERT_FALSE_MESSAGE(isCheck(&game), fens[i]);
        TEST_ASSERT_FALSE_MESSAGE(isCheckmate(&game), fens[i]);
    }
}

static void test_checkmate() {
    char fens[][90] = {
        "8/8/8/2rrr3/2rkb3/2rb4/5B2/6K1 b - - 0 1",                      // King can't move
        "1q5k/q1q5/8/8/8/8/1K6/8 w - - 0 1",                             // King can't move
        "8/8/8/2rrr3/2rkb3/2rbP2Q/8/6K1 b - - 0 1",                      // King can't move, checking piece is defended
        "8/8/8/2rrrN2/2rkb3/2rbn3/2N5/6K1 b - - 0 1",                    // Double check
        "8/6q1/8/8/4k1B1/8/1r6/r5K1 w - - 0 1",                          // Intercept is pinned
        "1q5k/2r5/8/1pPn4/1nKn4/1nnn4/8/8 w - b6 0 1",                   // Saving en-passant is pinned
        "8/3Bk2P/N7/PPP4Q/1K3r2/7r/NP4P1/4B3 w - - 0 1",                 // No interception possible
        "3rk2r/pppp1ppp/8/8/1B2Q3/8/PPPPPPPP/RN2KBNR b KQk - 0 1",       // No castling possible
        "7r/6r1/8/8/7K/8/8/6k1 w - - 0 1",                               // King can move but always checked
        "k7/8/8/8/5B2/5B2/8/1K4Q1 b - - 0 1",                            // King can move but always checked
        "8/8/8/8/8/1K6/8/1k5R b - - 0 1",                                // Blocked by other King
        "3rkbnr/1p1bp3/1q1p3p/p5pQ/3n4/PPR5/5PPP/6K1 b - - 2 2",         // Attacked diagonal, pawn can't intercept
        "8/5r2/4K1q1/4p3/3k4/8/8/8 w - - 0 7",                           // Stolen from https://github.com/jhlywa/chess.js
        "4r2r/p6p/1pnN2p1/kQp5/3pPq2/3P4/PPP3PP/R5K1 b - - 0 2",         // Stolen from https://github.com/jhlywa/chess.js
        "r3k2r/ppp2p1p/2n1p1p1/8/2B2P1q/2NPb1n1/PP4PP/R2Q3K w kq - 0 8", // Stolen from https://github.com/jhlywa/chess.js
        "8/6R1/pp1r3p/6p1/P3R1Pk/1P4P1/7K/8 b - - 0 4",                  // Stolen from https://github.com/jhlywa/chess.js
    };

    for (uint8_t i = 0; i < sizeof(fens) / sizeof(fens[0]); i++) {
        Game game;
        initializeFromFEN(&game, fens[i]);
        TEST_ASSERT_TRUE_MESSAGE(isCheck(&game), fens[i]);
        TEST_ASSERT_TRUE_MESSAGE(isCheckmate(&game), fens[i]);
    }
}

static void test_checkmate_bnilsou() {
    FILE* file;
    int bufferLength = 90;
    char buffer[bufferLength];

    file = fopen("test/data/bnilsou.fen", "r");
    if (file == NULL)
        TEST_ASSERT_TRUE_MESSAGE(false, "Failed to open test file");

    while (fgets(buffer, bufferLength, file)) {
        Game game;
        initializeFromFEN(&game, buffer);
        TEST_ASSERT_TRUE_MESSAGE(isCheck(&game), buffer);
        TEST_ASSERT_TRUE_MESSAGE(isCheckmate(&game), buffer);
    }

    fclose(file);
}

void run_check() {
    UNITY_BEGIN();

    RUN_TEST(test_check);
    RUN_TEST(test_notCheck);
    RUN_TEST(test_checkmate);
    RUN_TEST(test_checkmate_bnilsou);

    UNITY_END();
}