#include "unity.h"
#include "Tic/Tic.h"
#include "Utils/ConcurrentAccessFlag.h"

// Declaration of tested functions to avoid pulling the entire
// header with unnecessary dependencies

uint64_t Hal_GetElapsedTimeInNs(void);


// Mock functions, reduced to the minimum set

void ConcurrentAccessFlag_reset(ConcurrentAccessFlag *const self)
{
    // Assume no interrupt, NOP
}

bool ConcurrentAccessFlag_check(ConcurrentAccessFlag *const self)
{
    return false; // Assume no interrupt 
}

void ConcurrentAccessFlag_set(ConcurrentAccessFlag *const self)
{
    // Assume no interrupt, NOP
}

static uint32_t g_counter_value = 0;

uint32_t Tic_getCounterValue(const Tic *const tic, const Tic_Channel channel)
{
    return g_counter_value;
}


void setUp(void)
{
    g_counter_value = 0;
}

void tearDown(void)
{
    /* NOP */
}

void test_dummy()
{
    const uint64_t time = Hal_GetElapsedTimeInNs();
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_dummy);

    return UNITY_END();
}