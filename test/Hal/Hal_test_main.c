/**
 * @file    Hal_test_main.c
 * @brief   Hardware and RTOS independent tests for Hal.
 *
 * Tests for Hal_GetElapsedTimeInNs
 * 
 * Test data is supplied through:
 *   - g_counter_value        → mocked Tic_getCounterValue return value
 *   - Hal_SetReloadsCounter  → sets the internal reload counter directly
 *   - Hal_SetNsPerTickQ32    → sets the Q32.32 nanoseconds-per-tick factor
 *
 * Tests are grouped as follows:
 *   1. Zero / trivial values
 *   2. Counter at hardware boundary values
 *   3. Reload counter arithmetic
 *   4. total_ticks crossing the 32-bit boundary (exercises ticks_high path)
 *   5. Monotonicity
 *   6. Scaling with different ns_per_tick values
 *   7. SAMV71 realistic timing — including overflow boundary documentation
 *   8. Large / maximum values
 *
 */

#include "unity.h"
#include "Tic/Tic.h"
#include "Utils/ConcurrentAccessFlag.h"
#include "Hal.h"
#include "HalInternal.h"

#include <stdint.h>

// Constants

#define TICKS_PER_RELOAD  65535ULL

/*
 * Q32.32 representations of nanoseconds-per-tick.
 * With Q32_1NS the formula reduces to:  result_ns == total_ticks  (exact).
 */
#define Q32_1NS   (1ULL << 32)   /* 1 ns per tick  – no overflow possible  */
#define Q32_2NS   (2ULL << 32)   /* 2 ns per tick  – scaling test          */
#define Q32_10NS  (10ULL << 32)  /* 10 ns per tick – larger scaling test   */

/*
 * SAMV71 @ 300 MHz / prescaler 8 = 37.5 MHz timer clock.
 * ns_per_tick = 1e9 / 37.5e6 = 26.666... ns
 * Q32 value   = (1e9 << 32) / 37500000 = 114 532 461 226  (truncated)
 */
#define SAMV71_PRESCALED_CLK_HZ  37500000ULL
#define Q32_SAMV71  ((1000000000ULL << 32) / SAMV71_PRESCALED_CLK_HZ)

// Mocks

static uint32_t g_counter_value = 0;

void ConcurrentAccessFlag_reset(ConcurrentAccessFlag *const self) { (void)self; }
void ConcurrentAccessFlag_set(ConcurrentAccessFlag *const self)   { (void)self; }

bool ConcurrentAccessFlag_check(ConcurrentAccessFlag *const self)
{
    (void)self;
    return false; /* no concurrent interrupt in tests */
}

uint32_t Tic_getCounterValue(const Tic *const tic, const Tic_Channel channel)
{
    (void)tic; (void)channel;
    return g_counter_value;
}

static void set_state(uint32_t reloads, uint32_t counter)
{
    Hal_SetReloadsCounter(reloads);
    g_counter_value = counter;
}

/*
 * Reference computation using 128-bit arithmetic to avoid overflow.
 * Provides the mathematically correct result for comparison with the
 * 64-bit split implementation.
 */
static uint64_t ref_ns(uint64_t ns_ptq, uint32_t reloads, uint32_t counter)
{
    const uint64_t total = (uint64_t)reloads * TICKS_PER_RELOAD + counter;
    return (uint64_t)(((__uint128_t)total * (__uint128_t)ns_ptq) >> 32);
}

void setUp(void)
{
    /* Default: 1 ns per tick so expected_ns == total_ticks (exact math). */
    Hal_SetNsPerTickQ32(Q32_1NS);
    Hal_SetReloadsCounter(0);
    g_counter_value = 0;
}

void tearDown(void) { /* NOP */ }

/* ===================================================================== */
/* GROUP 1 – Zero and trivial single-tick values                        */
/* ===================================================================== */

void test_zero_state_returns_zero(void)
{
    set_state(0, 0);
    TEST_ASSERT_EQUAL_UINT64(0, Hal_GetElapsedTimeInNs());
}

void test_single_tick_returns_one_ns(void)
{
    set_state(0, 1);
    TEST_ASSERT_EQUAL_UINT64(1, Hal_GetElapsedTimeInNs());
}

void test_small_counter_value(void)
{
    set_state(0, 1000);
    TEST_ASSERT_EQUAL_UINT64(1000, Hal_GetElapsedTimeInNs());
}

void test_zero_ns_per_tick_always_returns_zero(void)
{
    Hal_SetNsPerTickQ32(0);
    set_state(100, 50000);
    TEST_ASSERT_EQUAL_UINT64(0, Hal_GetElapsedTimeInNs());
}

/* ===================================================================== */
/* GROUP 2 – Counter at hardware boundary values                        */
/* ===================================================================== */

void test_counter_just_below_ticks_per_reload(void)
{
    /* 65 534 = TICKS_PER_RELOAD - 1: highest sample before the reload fires */
    set_state(0, 65534);
    TEST_ASSERT_EQUAL_UINT64(65534, Hal_GetElapsedTimeInNs());
}

void test_counter_exactly_at_ticks_per_reload(void)
{
    /* RC = 65 535: the reload interrupt fires when counter == RC */
    set_state(0, 65535);
    TEST_ASSERT_EQUAL_UINT64(65535, Hal_GetElapsedTimeInNs());
}

void test_counter_above_reload_boundary_in_mock(void)
{
    /*
     * Hardware counter is 16-bit and cannot exceed 65 535, but the mock
     * accepts any uint32.  This tests arithmetic robustness for large
     * counter values that should never occur on real hardware.
     */
    set_state(0, 65536);
    TEST_ASSERT_EQUAL_UINT64(65536, Hal_GetElapsedTimeInNs());
}

/* ===================================================================== */
/* GROUP 3 – Reload counter arithmetic                                  */
/* ===================================================================== */

void test_one_reload_zero_ticks(void)
{
    set_state(1, 0);
    TEST_ASSERT_EQUAL_UINT64(TICKS_PER_RELOAD, Hal_GetElapsedTimeInNs());
}

void test_one_reload_one_tick(void)
{
    set_state(1, 1);
    TEST_ASSERT_EQUAL_UINT64(TICKS_PER_RELOAD + 1, Hal_GetElapsedTimeInNs());
}

void test_reload_boundary_continuity(void)
{
    /*
     * (reloads=0, counter=65535)  and  (reloads=1, counter=0)  must both
     * produce the same total_ticks (= 65535) because the reload fires at
     * counter == TICKS_PER_RELOAD, making 1*65535+0 == 0*65535+65535.
     * A wrong TICKS_PER_RELOAD constant (e.g. 65536) would break this.
     */
    set_state(0, 65535);
    const uint64_t before_reload = Hal_GetElapsedTimeInNs();

    set_state(1, 0);
    const uint64_t after_reload = Hal_GetElapsedTimeInNs();

    TEST_ASSERT_EQUAL_UINT64(before_reload, after_reload);
}

void test_hundred_reloads_zero_ticks(void)
{
    set_state(100, 0);
    TEST_ASSERT_EQUAL_UINT64(100 * TICKS_PER_RELOAD, Hal_GetElapsedTimeInNs());
}

void test_hundred_reloads_with_counter(void)
{
    set_state(100, 12345);
    TEST_ASSERT_EQUAL_UINT64(100 * TICKS_PER_RELOAD + 12345,
                             Hal_GetElapsedTimeInNs());
}

void test_each_additional_reload_adds_exactly_ticks_per_reload_ns(void)
{
    set_state(5, 100);
    const uint64_t t5 = Hal_GetElapsedTimeInNs();

    set_state(6, 100);
    const uint64_t t6 = Hal_GetElapsedTimeInNs();

    TEST_ASSERT_EQUAL_UINT64(TICKS_PER_RELOAD, t6 - t5);
}

/* ===================================================================== */
/* GROUP 4 – total_ticks crossing the 32-bit boundary                   */
/* (exercises the ticks_high != 0 code path)                            */
/* ===================================================================== */

void test_total_ticks_just_above_uint32_max(void)
{
    /*
     * reloads = 65 539, counter = 0:
     * total = 65539 * 65535 = 4 295 098 365 > UINT32_MAX (4 294 967 295)
     * → ticks_high = 1
     */
    set_state(65539, 0);
    const uint64_t expected = (uint64_t)65539 * TICKS_PER_RELOAD;
    TEST_ASSERT_EQUAL_UINT64(expected, Hal_GetElapsedTimeInNs());
}

void test_total_ticks_just_above_uint32_max_with_counter(void)
{
    set_state(65539, 42);
    const uint64_t expected = (uint64_t)65539 * TICKS_PER_RELOAD + 42;
    TEST_ASSERT_EQUAL_UINT64(expected, Hal_GetElapsedTimeInNs());
}

void test_ticks_low_is_max_uint32(void)
{
    /*
     * reloads = 65 537, counter = 0:
     * total = 65537 * 65535 = (65536+1)*(65536-1) = 65536^2 - 1 = 2^32 - 1
     *       = 0xFFFFFFFF
     * → ticks_high = 0, ticks_low = 0xFFFFFFFF
     *
     * With Q32_1NS this is safe: 0xFFFFFFFF * 2^32 = 2^64 - 2^32 < UINT64_MAX.
     * Expected result with 1 ns/tick == total_ticks == 0xFFFFFFFF.
     */
    set_state(65537, 0);
    TEST_ASSERT_EQUAL_UINT64(0xFFFFFFFFULL, Hal_GetElapsedTimeInNs());
}

void test_large_ticks_high_value(void)
{
    /*
     * Use a reload count that sets ticks_high > 1 to exercise the high
     * multiplication path with a non-trivial multiplier.
     * reloads = 131074 (= 2 * 65537), counter = 0
     * total = 131074 * 65535 = 2 * (2^32 - 1) = 2^33 - 2
     * ticks_high = 1, ticks_low = 0xFFFFFFFE
     */
    set_state(131074, 0);
    const uint64_t expected = (uint64_t)131074 * TICKS_PER_RELOAD;
    TEST_ASSERT_EQUAL_UINT64(expected, Hal_GetElapsedTimeInNs());
}

/* ===================================================================== */
/* GROUP 5 – Monotonicity                                                */
/* ===================================================================== */

void test_monotonically_increasing_counter(void)
{
    set_state(0, 0);     const uint64_t t0 = Hal_GetElapsedTimeInNs();
    set_state(0, 1);     const uint64_t t1 = Hal_GetElapsedTimeInNs();
    set_state(0, 1000);  const uint64_t t2 = Hal_GetElapsedTimeInNs();
    set_state(0, 65534); const uint64_t t3 = Hal_GetElapsedTimeInNs();

    TEST_ASSERT_GREATER_THAN_UINT64(t0, t1);
    TEST_ASSERT_GREATER_THAN_UINT64(t1, t2);
    TEST_ASSERT_GREATER_THAN_UINT64(t2, t3);
}

void test_monotonically_increasing_reloads(void)
{
    set_state(0,     0); const uint64_t t0 = Hal_GetElapsedTimeInNs();
    set_state(1,     0); const uint64_t t1 = Hal_GetElapsedTimeInNs();
    set_state(100,   0); const uint64_t t2 = Hal_GetElapsedTimeInNs();
    set_state(65539, 0); const uint64_t t3 = Hal_GetElapsedTimeInNs();

    TEST_ASSERT_GREATER_THAN_UINT64(t0, t1);
    TEST_ASSERT_GREATER_THAN_UINT64(t1, t2);
    TEST_ASSERT_GREATER_THAN_UINT64(t2, t3);
}

void test_strictly_increases_across_reload_boundary(void)
{
    set_state(4, 65534); const uint64_t before = Hal_GetElapsedTimeInNs();
    set_state(4, 65535); const uint64_t at_rc  = Hal_GetElapsedTimeInNs();
    set_state(5, 1);     const uint64_t after  = Hal_GetElapsedTimeInNs();

    TEST_ASSERT_GREATER_THAN_UINT64(before, at_rc);
    TEST_ASSERT_GREATER_THAN_UINT64(at_rc,  after);
}

/* ===================================================================== */
/* GROUP 6 – Scaling with different ns_per_tick values                  */
/* ===================================================================== */

void test_doubling_ns_per_tick_doubles_result(void)
{
    set_state(10, 1234);

    Hal_SetNsPerTickQ32(Q32_1NS);
    const uint64_t result_1ns = Hal_GetElapsedTimeInNs();

    Hal_SetNsPerTickQ32(Q32_2NS);
    const uint64_t result_2ns = Hal_GetElapsedTimeInNs();

    TEST_ASSERT_EQUAL_UINT64(result_1ns * 2, result_2ns);
}

void test_ten_times_ns_per_tick_gives_ten_times_result(void)
{
    set_state(3, 500);

    Hal_SetNsPerTickQ32(Q32_1NS);
    const uint64_t result_1ns = Hal_GetElapsedTimeInNs();

    Hal_SetNsPerTickQ32(Q32_10NS);
    const uint64_t result_10ns = Hal_GetElapsedTimeInNs();

    TEST_ASSERT_EQUAL_UINT64(result_1ns * 10, result_10ns);
}

void test_two_ns_per_tick_with_explicit_counter(void)
{
    set_state(0, 100);
    Hal_SetNsPerTickQ32(Q32_2NS);
    TEST_ASSERT_EQUAL_UINT64(200, Hal_GetElapsedTimeInNs());
}

/* ===================================================================== */
/* GROUP 7 – SAMV71 realistic timing                                    */
/* ===================================================================== */

void test_samv71_one_ms_within_tolerance(void)
{
    /*
     * 37 500 ticks at 37.5 MHz = exactly 1 ms in theory (1 000 000 ns).
     * Q32 truncation introduces at most 1 ns per tick of error.
     * For 37 500 ticks the cumulative error is < 1 000 ns → delta = 1000.
     */
    Hal_SetNsPerTickQ32(Q32_SAMV71);
    set_state(0, 37500);
    const uint64_t result = Hal_GetElapsedTimeInNs();
    TEST_ASSERT_UINT64_WITHIN(1000, 1000000ULL, result);
}

void test_samv71_small_reload_count_matches_reference(void)
{
    /*
     * 1 000 reloads, 0 ticks: ticks_low = 65 535 000 < overflow threshold.
     * The 64-bit split and the 128-bit reference must agree exactly.
     */
    Hal_SetNsPerTickQ32(Q32_SAMV71);
    set_state(1000, 0);
    TEST_ASSERT_EQUAL_UINT64(ref_ns(Q32_SAMV71, 1000, 0),
                             Hal_GetElapsedTimeInNs());
}

void test_samv71_ticks_high_path_matches_reference(void)
{
    /*
     * reloads = 65 538, counter = 0:
     * total = 4 295 032 830 → ticks_high = 1, ticks_low = 65 534.
     * ticks_low * Q32_SAMV71 ≈ 7.5e15 << UINT64_MAX: no overflow.
     */
    Hal_SetNsPerTickQ32(Q32_SAMV71);
    set_state(65538, 0);
    TEST_ASSERT_EQUAL_UINT64(ref_ns(Q32_SAMV71, 65538, 0),
                             Hal_GetElapsedTimeInNs());
}

void test_samv71_overflow_boundary(void)
{
    /*
     * reloads = 65 537, counter = 0:
     *   total_ticks = 65537 * 65535 = 2^32 - 1 = 0xFFFFFFFF
     *   ticks_high  = 0, ticks_low = 0xFFFFFFFF
     *
     * With SAMV71 timing (Q32_SAMV71 ≈ 1.14e11):
     *   A naive ticks_low * ns_per_tick_q32 would overflow uint64_t.
     *   The fixed implementation splits ns_per_tick_q32 into 32-bit halves
     *   so all intermediates stay within 64 bits.  Result must match the
     *   128-bit reference exactly.
     */
    Hal_SetNsPerTickQ32(Q32_SAMV71);
    set_state(65537, 0);
    const uint64_t expected = ref_ns(Q32_SAMV71, 65537, 0);
    TEST_ASSERT_EQUAL_UINT64(expected, Hal_GetElapsedTimeInNs());
}

/* ===================================================================== */
/* GROUP 8 – Large / maximum values (using safe Q32_1NS)                */
/* ===================================================================== */

void test_max_reloads_zero_counter(void)
{
    set_state(UINT32_MAX, 0);
    const uint64_t expected = (uint64_t)UINT32_MAX * TICKS_PER_RELOAD;
    TEST_ASSERT_EQUAL_UINT64(expected, Hal_GetElapsedTimeInNs());
}

void test_max_reloads_max_hardware_counter(void)
{
    set_state(UINT32_MAX, 65535);
    const uint64_t expected = (uint64_t)UINT32_MAX * TICKS_PER_RELOAD + 65535;
    TEST_ASSERT_EQUAL_UINT64(expected, Hal_GetElapsedTimeInNs());
}

void test_large_counter_with_many_reloads(void)
{
    set_state(0xFFFF, 0x8000);
    const uint64_t expected = (uint64_t)0xFFFF * TICKS_PER_RELOAD + 0x8000;
    TEST_ASSERT_EQUAL_UINT64(expected, Hal_GetElapsedTimeInNs());
}

/* ===================================================================== */
/* main                                                                  */
/* ===================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* GROUP 1 – Zero / trivial */
    RUN_TEST(test_zero_state_returns_zero);
    RUN_TEST(test_single_tick_returns_one_ns);
    RUN_TEST(test_small_counter_value);
    RUN_TEST(test_zero_ns_per_tick_always_returns_zero);

    /* GROUP 2 – Counter boundary */
    RUN_TEST(test_counter_just_below_ticks_per_reload);
    RUN_TEST(test_counter_exactly_at_ticks_per_reload);
    RUN_TEST(test_counter_above_reload_boundary_in_mock);

    /* GROUP 3 – Reload arithmetic */
    RUN_TEST(test_one_reload_zero_ticks);
    RUN_TEST(test_one_reload_one_tick);
    RUN_TEST(test_reload_boundary_continuity);
    RUN_TEST(test_hundred_reloads_zero_ticks);
    RUN_TEST(test_hundred_reloads_with_counter);
    RUN_TEST(test_each_additional_reload_adds_exactly_ticks_per_reload_ns);

    /* GROUP 4 – 32-bit boundary */
    RUN_TEST(test_total_ticks_just_above_uint32_max);
    RUN_TEST(test_total_ticks_just_above_uint32_max_with_counter);
    RUN_TEST(test_ticks_low_is_max_uint32);
    RUN_TEST(test_large_ticks_high_value);

    /* GROUP 5 – Monotonicity */
    RUN_TEST(test_monotonically_increasing_counter);
    RUN_TEST(test_monotonically_increasing_reloads);
    RUN_TEST(test_strictly_increases_across_reload_boundary);

    /* GROUP 6 – Scaling */
    RUN_TEST(test_doubling_ns_per_tick_doubles_result);
    RUN_TEST(test_ten_times_ns_per_tick_gives_ten_times_result);
    RUN_TEST(test_two_ns_per_tick_with_explicit_counter);

    /* GROUP 7 – SAMV71 realistic timing */
    RUN_TEST(test_samv71_one_ms_within_tolerance);
    RUN_TEST(test_samv71_small_reload_count_matches_reference);
    RUN_TEST(test_samv71_ticks_high_path_matches_reference);
    RUN_TEST(test_samv71_overflow_boundary);

    /* GROUP 8 – Large / maximum values */
    RUN_TEST(test_max_reloads_zero_counter);
    RUN_TEST(test_max_reloads_max_hardware_counter);
    RUN_TEST(test_large_counter_with_many_reloads);

    return UNITY_END();
}