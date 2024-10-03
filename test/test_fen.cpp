#include <unity.h>
#include <chess.h>
#include "utils.h"

static void test_initFromFen_castling() {
    Game game;
    initializeFromFEN(&game, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    TEST_ASSERT_TRUE(game.state.castlingK[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::Black]);
    TEST_ASSERT_EQUAL(0, game.halfmoveClock);
    TEST_ASSERT_EQUAL(1, game.fullmoveClock);

    initializeFromFEN(&game, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Kk - 4 12");

    TEST_ASSERT_TRUE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::Black]);
    TEST_ASSERT_EQUAL(4, game.halfmoveClock);
    TEST_ASSERT_EQUAL(12, game.fullmoveClock);

    initializeFromFEN(&game, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Qq - 0 8");

    TEST_ASSERT_FALSE(game.state.castlingK[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::Black]);
    TEST_ASSERT_EQUAL(0, game.halfmoveClock);
    TEST_ASSERT_EQUAL(8, game.fullmoveClock);

    initializeFromFEN(&game, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 99 50");

    TEST_ASSERT_FALSE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::Black]);
    TEST_ASSERT_EQUAL(99, game.halfmoveClock);
    TEST_ASSERT_EQUAL(50, game.fullmoveClock);
}

static void test_writeToFen_castling() {
    Game game;
    initializeGame(&game, DEFAULT_SENSORS_STATE);

    char fenBuffer[96];
    TEST_ASSERT_EQUAL(56, writeToFEN(&game, fenBuffer));
    TEST_ASSERT_EQUAL_STRING("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", fenBuffer);

    game.state.castlingK[bits::White] = false;
    game.halfmoveClock = 9;
    game.fullmoveClock = 8;
    TEST_ASSERT_EQUAL(55, writeToFEN(&game, fenBuffer));
    TEST_ASSERT_EQUAL_STRING("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Qkq - 9 8", fenBuffer);

    game.state.castlingK[bits::Black] = false;
    game.halfmoveClock = 2;
    game.fullmoveClock = 3;
    TEST_ASSERT_EQUAL(54, writeToFEN(&game, fenBuffer));
    TEST_ASSERT_EQUAL_STRING("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Qq - 2 3", fenBuffer);

    game.state.castlingQ[bits::Black] = false;
    game.halfmoveClock = 99;
    game.fullmoveClock = 50;
    TEST_ASSERT_EQUAL(55, writeToFEN(&game, fenBuffer));
    TEST_ASSERT_EQUAL_STRING("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Q - 99 50", fenBuffer);

    game.state.castlingQ[bits::White] = false;
    game.halfmoveClock = 0;
    game.fullmoveClock = 3;
    TEST_ASSERT_EQUAL(53, writeToFEN(&game, fenBuffer));
    TEST_ASSERT_EQUAL_STRING("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 3", fenBuffer);
}

static void test_writeToFen_game() {
    Game game;
    initializeGame(&game, DEFAULT_SENSORS_STATE);

    const char* actions = {
        "# G.Kasparov - V.Topalov, Netherlands, 1999 (1-0)\n"
        "# https://www.chess.com/games/view/969971\n"
        "-e2 +e4 -d7 +d6"
        "-d2 +d4 -g8 +f6"
        "-b1 +c3 -g7 +g6"
        "-c1 +e3 -f8 +g7"
        "-d1 +d2 -c7 +c6"
        "-f2 +f3 -b7 +b5"
        "-g1 +e2 -b8 +d7"
        "-e3 +h6 -g7 -h6 +h6"
        "-d2 -h6 +h6 -c8 +b7"
        "-a2 +a3 -e7 +e5"
        "-e1 +c1 -a1 +d1"
        "# Black to move to continue!"
    };
    EXEC_REF(&game, actions, DEFAULT_SENSORS_STATE);

    char fenBuffer[96];
    TEST_ASSERT_EQUAL(70, writeToFEN(&game, fenBuffer));
    TEST_ASSERT_EQUAL_STRING("r2qk2r/pb1n1p1p/2pp1npQ/1p2p3/3PP3/P1N2P2/1PP1N1PP/2KR1B1R b kq - 1 11", fenBuffer);
}

void run_fen()
{
    UNITY_BEGIN();

    RUN_TEST(test_initFromFen_castling);
    RUN_TEST(test_writeToFen_castling);
    RUN_TEST(test_writeToFen_game);

    UNITY_END();
}