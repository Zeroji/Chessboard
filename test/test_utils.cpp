#include <unity.h>
#include <utils.h>

static void test_timeDiff() {
    TEST_ASSERT_EQUAL(200, timeDiff(0, 200));

    // Overflow
    TEST_ASSERT_EQUAL(265, timeDiff(0xFFFFFFFF - 250, 15));
}

static void test_stabilize() {
    uint64_t state = stabilizeValue(0, 0, 5);
    TEST_ASSERT_EQUAL(0, state);

    // Still stable
    for (uint8_t i = 1; i < 5; i++) {
        state = stabilizeValue(1, i, 5);
        TEST_ASSERT_EQUAL(0, state);
    }

    // Update
    state = stabilizeValue(1, 5, 5);
    TEST_ASSERT_EQUAL(1, state);

    // Still stable
    state = stabilizeValue(2, 6, 5);
    TEST_ASSERT_EQUAL(1, state);

    // Update
    state = stabilizeValue(2, 10, 5);
    TEST_ASSERT_EQUAL(2, state);

    // Direct update after "long" time
    state = stabilizeValue(3, 16, 5);
    TEST_ASSERT_EQUAL(3, state);
}

void run_utils() {
    UNITY_BEGIN();

    RUN_TEST(test_timeDiff);
    RUN_TEST(test_stabilize);

    UNITY_END();
}