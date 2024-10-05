#include <unity.h>
#include <chess.h>
#include "utils.h"

static void test_check() {
    char fens[][90] = {
        "r6r/1b2k1bq/8/8/7B/8/8/R3K2R b KQ - 3 2", // Bishop
        "8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3", // Pawn
        "r1b1kbnr/pp1ppppp/n7/q1p5/8/P2P1N2/1PP1PPPP/RNBQKB1R w KQkq - 2 2", // Queen
        "r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQkq - 3 2", // Queen
        "2kr3r/p1ppqpb1/bN2Qnp1/3P4/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 3 2", // Knight
        "rnb2k2/pp1Pbppp/2p5/q7/2B5/6n1/PPPQN1PP/RNB1K2r w Q - 3 9", // Rook
        "2r5/3pk3/1P6/8/8/2K5/8/8 w - - 5 4", // Rook
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1Q1PP/R4RK1 w - - 0 10", // Bishop
        "3k4/3pPK2/8/7r/8/8/8/8 b - - 0 1", // Pawn
        "r3k2r/1b5q/8/8/8/2b5/7B/R3K2R w KQkq - 0 1", // Bishop
        "8/8/2k5/5q2/8/3n4/5K2/8 w - - 0 1", // Knight + Queen
    };

    for(uint8_t i = 0; i < sizeof(fens) / sizeof(fens[0]); i++)
    {
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

    for(uint8_t i = 0; i < sizeof(fens) / sizeof(fens[0]); i++)
    {
        Game game;
        initializeFromFEN(&game, fens[i]);
        TEST_ASSERT_FALSE_MESSAGE(isCheck(&game), fens[i]);
        TEST_ASSERT_FALSE_MESSAGE(isCheckmate(&game), fens[i]);
    }
}

void run_check()
{
    UNITY_BEGIN();

    RUN_TEST(test_check);
    RUN_TEST(test_notCheck);

    UNITY_END();
}