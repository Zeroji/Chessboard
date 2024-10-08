#include "mock_sensors.h"
#include "utils.h"
#include <chess.h>
#include <unity.h>

static void test_promotionWhite() {
    Game game;
    initializeFromFEN(&game, "8/6P1/3k4/8/8/8/1pK5/R7 w - - 0 1");
    uint64_t sensorsState = extractSensorsState(&game);

    EXEC(&game, "-g7 +g8", sensorsState);

    TEST_ASSERT_EQUAL(bits::Black | bits::ToPlay, game.state.status);
    TEST_ASSERT_FALSE(game.lastMoveW.captured);
    TEST_ASSERT_FALSE(game.lastMoveW.check);
    TEST_ASSERT_TRUE(game.lastMoveW.promotion);
    TEST_ASSERT_EQUAL(EPiece::WPawn, game.lastMoveW.piece);
    TEST_ASSERT_EQUAL(6 * 8 + 6, game.lastMoveW.start);
    TEST_ASSERT_EQUAL(7 * 8 + 6, game.lastMoveW.end);
    TEST_ASSERT_EQUAL(EPiece::WQueen, game.board[7 * 8 + 6]);
}

static void test_promotionBlack() {
    Game game;
    initializeFromFEN(&game, "8/6P1/3k4/8/8/8/1pK5/R7 b - - 0 1");
    uint64_t sensorsState = extractSensorsState(&game);

    EXEC(&game, "-b2 -a1 +a1", sensorsState);

    TEST_ASSERT_EQUAL(bits::White | bits::ToPlay, game.state.status);
    TEST_ASSERT_TRUE(game.lastMoveB.captured);
    TEST_ASSERT_FALSE(game.lastMoveB.check);
    TEST_ASSERT_TRUE(game.lastMoveB.promotion);
    TEST_ASSERT_EQUAL(EPiece::BPawn, game.lastMoveB.piece);
    TEST_ASSERT_EQUAL(1 * 8 + 1, game.lastMoveB.start);
    TEST_ASSERT_EQUAL(0 * 8 + 0, game.lastMoveB.end);
    TEST_ASSERT_EQUAL(EPiece::BQueen, game.board[0 * 8 + 0]);
}

void run_promotion() {
    UNITY_BEGIN();

    RUN_TEST(test_promotionWhite);
    RUN_TEST(test_promotionBlack);

    UNITY_END();
}