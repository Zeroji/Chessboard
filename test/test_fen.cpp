#include <unity.h>
#include <chess.h>
#include <sensors.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_writeToFen() {
    Game game;
    initializeGame(&game, DEFAULT_SENSORS_STATE);

    char fenBuffer[96];
    int res = writeToFEN(&game, fenBuffer);

    TEST_ASSERT_EQUAL(56, res);
    TEST_ASSERT_EQUAL_STRING("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", fenBuffer);
}

int main( int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_writeToFen);

    UNITY_END();
}