#include "mock_sensors.h"
#include "utils.h"
#include <chess.h>
#include <unity.h>

static void test_castlingKingSide() {
    Game game;
    initializeFromFEN(&game, "rnbqk2r/pppppppp/8/8/8/8/PPPPPPPP/RNBQK2R w KQkq - 0 1");
    uint64_t sensorsState = extractSensorsState(&game);

    sensorsState = EXEC(&game, "-e1 +g1 -h1 +f1", sensorsState);

    TEST_ASSERT_EQUAL(bits::Black | bits::ToPlay, game.state.status);
    TEST_ASSERT_FALSE(game.lastMoveW.captured);
    TEST_ASSERT_FALSE(game.lastMoveW.check);
    TEST_ASSERT_FALSE(game.lastMoveW.promotion);
    TEST_ASSERT_EQUAL(EPiece::WKing, game.lastMoveW.piece);
    TEST_ASSERT_EQUAL(0 * 8 + 4, game.lastMoveW.start);
    TEST_ASSERT_EQUAL(0 * 8 + 6, game.lastMoveW.end);

    sensorsState = EXEC(&game, "-e8 +g8 -h8 +f8", sensorsState);

    TEST_ASSERT_EQUAL(bits::White | bits::ToPlay, game.state.status);
    TEST_ASSERT_FALSE(game.lastMoveB.captured);
    TEST_ASSERT_FALSE(game.lastMoveB.check);
    TEST_ASSERT_FALSE(game.lastMoveB.promotion);
    TEST_ASSERT_EQUAL(EPiece::BKing, game.lastMoveB.piece);
    TEST_ASSERT_EQUAL(7 * 8 + 4, game.lastMoveB.start);
    TEST_ASSERT_EQUAL(7 * 8 + 6, game.lastMoveB.end);
}

static void test_castlingQueenSide() {
    Game game;
    initializeFromFEN(&game, "r3kbnr/pppppppp/8/8/8/8/PPPPPPPP/R3KBNR b KQkq - 0 1");
    uint64_t sensorsState = extractSensorsState(&game);

    sensorsState = EXEC(&game, "-e8 +c8 -a8 +d8", sensorsState);

    TEST_ASSERT_EQUAL(bits::White | bits::ToPlay, game.state.status);
    TEST_ASSERT_FALSE(game.lastMoveB.captured);
    TEST_ASSERT_FALSE(game.lastMoveB.check);
    TEST_ASSERT_FALSE(game.lastMoveB.promotion);
    TEST_ASSERT_EQUAL(EPiece::BKing, game.lastMoveB.piece);
    TEST_ASSERT_EQUAL(7 * 8 + 4, game.lastMoveB.start);
    TEST_ASSERT_EQUAL(7 * 8 + 2, game.lastMoveB.end);

    sensorsState = EXEC(&game, "-e1 +c1 -a1 +d1", sensorsState);

    TEST_ASSERT_EQUAL(bits::Black | bits::ToPlay, game.state.status);
    TEST_ASSERT_FALSE(game.lastMoveW.captured);
    TEST_ASSERT_FALSE(game.lastMoveW.check);
    TEST_ASSERT_FALSE(game.lastMoveW.promotion);
    TEST_ASSERT_EQUAL(EPiece::WKing, game.lastMoveW.piece);
    TEST_ASSERT_EQUAL(0 * 8 + 4, game.lastMoveW.start);
    TEST_ASSERT_EQUAL(0 * 8 + 2, game.lastMoveW.end);
}

static void test_castlingAvailability_KingMoved() {
    const char* fen = "r3k2r/p6p/8/3BB3/3bb3/8/P6P/R3K2R w KQkq - 0 1";
    Game game;
    initializeFromFEN(&game, fen);
    uint64_t sensorsState = extractSensorsState(&game);

    TEST_ASSERT_TRUE(game.state.castlingK[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::Black]);

    sensorsState = EXEC(&game, "-e1 +e2", sensorsState);

    TEST_ASSERT_FALSE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::Black]);

    sensorsState = EXEC(&game, "-e8 +c8 -a8 +d8", sensorsState);

    TEST_ASSERT_FALSE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::Black]);
}

static void test_castlingAvailability_RookMoved() {
    const char* fen = "r3k2r/p6p/8/3BB3/3bb3/8/P6P/R3K2R w KQkq - 0 1";
    Game game;
    initializeFromFEN(&game, fen);
    uint64_t sensorsState = extractSensorsState(&game);

    TEST_ASSERT_TRUE(game.state.castlingK[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::Black]);

    sensorsState = EXEC(&game, "-a1 +b1", sensorsState);

    TEST_ASSERT_TRUE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::Black]);

    sensorsState = EXEC(&game, "-a8 +b8", sensorsState);

    TEST_ASSERT_TRUE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::Black]);

    sensorsState = EXEC(&game, "-h1 +f1", sensorsState);

    TEST_ASSERT_FALSE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::Black]);

    sensorsState = EXEC(&game, "-h8 +f8", sensorsState);

    TEST_ASSERT_FALSE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::Black]);
}

static void test_castlingAvailability_RookCaptured() {
    const char* fen = "r3k2r/p6p/8/3BB3/3bb3/8/P6P/R3K2R w KQkq - 0 1";
    Game game;
    initializeFromFEN(&game, fen);
    uint64_t sensorsState = extractSensorsState(&game);

    TEST_ASSERT_TRUE(game.state.castlingK[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::Black]);

    sensorsState = EXEC(&game, "-d5 -a8 +a8", sensorsState);

    TEST_ASSERT_TRUE(game.state.castlingK[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::Black]);

    sensorsState = EXEC(&game, "-a1 -d4 +a1", sensorsState);

    TEST_ASSERT_TRUE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_TRUE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::Black]);

    sensorsState = EXEC(&game, "-h8 -e5 +h8", sensorsState);

    TEST_ASSERT_TRUE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::Black]);

    sensorsState = EXEC(&game, "-e4 -h1 +h1", sensorsState);

    TEST_ASSERT_FALSE(game.state.castlingK[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::White]);
    TEST_ASSERT_FALSE(game.state.castlingK[bits::Black]);
    TEST_ASSERT_FALSE(game.state.castlingQ[bits::Black]);
}

void run_castling() {
    UNITY_BEGIN();

    RUN_TEST(test_castlingKingSide);
    RUN_TEST(test_castlingQueenSide);
    RUN_TEST(test_castlingAvailability_KingMoved);
    RUN_TEST(test_castlingAvailability_RookMoved);
    RUN_TEST(test_castlingAvailability_RookCaptured);

    UNITY_END();
}