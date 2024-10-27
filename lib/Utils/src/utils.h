#include <stdint.h>

// Return time difference between timestamps
uint32_t timeDiff(uint32_t from_ms, uint32_t to_ms);

// Stabilize an uint64_t value
uint64_t stabilizeValue(uint64_t p_boardState, uint32_t p_now_ms, uint32_t p_delay);
