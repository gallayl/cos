#include "mock_freertos.h"

static EventBits_t s_bits = 0;
static int s_dummy_group = 1;

void mock_freertos_reset(void)
{
    s_bits = 0;
}

void mock_freertos_set_bits(EventBits_t bits)
{
    s_bits |= bits;
}

void mock_freertos_clear_bits(EventBits_t bits)
{
    s_bits &= ~bits;
}

EventBits_t mock_freertos_get_bits(void)
{
    return s_bits;
}

EventGroupHandle_t xEventGroupCreate(void)
{
    return (EventGroupHandle_t)&s_dummy_group;
}

EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToSet)
{
    (void)xEventGroup;
    s_bits |= uxBitsToSet;
    return s_bits;
}

EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToClear)
{
    (void)xEventGroup;
    EventBits_t prev = s_bits;
    s_bits &= ~uxBitsToClear;
    return prev;
}

EventBits_t xEventGroupGetBits(EventGroupHandle_t xEventGroup)
{
    (void)xEventGroup;
    return s_bits;
}

EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToWaitFor,
                                BaseType_t xClearOnExit, BaseType_t xWaitForAllBits, TickType_t xTicksToWait)
{
    (void)xEventGroup;
    (void)xClearOnExit;
    (void)xWaitForAllBits;
    (void)xTicksToWait;
    return s_bits & uxBitsToWaitFor;
}
