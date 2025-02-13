#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <stdio.h>
#include <math.h>

#if defined(MEASURE_SYSTICKS) && MEASURE_SYSTICKS > 0
#include "cpu.h"
#include "clk.h"
#endif

#if defined(MEASURE_HEAP) && MEASURE_HEAP > 0
#include "malloc_monitor.h"

static size_t _heap_before;
#endif

#if defined(MEASURE_SYSTICKS) && MEASURE_SYSTICKS > 0
static void _systick_start(uint32_t ticks) {
    /* disable SysTick, clear value */
    SysTick->CTRL = 0;
    SysTick->VAL = 0;
    /* prepare value in re-load register */
    SysTick->LOAD = ticks;
    /* start and wait for the load value to be applied */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    while (SysTick->VAL == 0) { /* Wait for SysTick to start and spin */ }
}

static void _systick_stop(void) {
    uint32_t ticks = SysTick_LOAD_RELOAD_Msk - SysTick->VAL;
    //uint32_t ticks_per_sec = coreclk();
    printf("=== %ld\n", ticks);
    //double nsecs = ((double) ticks / (double) ticks_per_sec) * 1000000000.0;

    //printf("Elapsed systicks: %ld\n", ticks);
    //printf("Core clock: %d MHz\n", (int) floor(ticks_per_sec/1000000.0));
    /*printf("Calculated nanoseconds: %f\n", nsecs);
    printf("Calculated microseconds: %f\n", nsecs/1000.0);
    printf("Calculated milliseconds: %f\n", nsecs/1000000.0);*/
}
#endif

#if defined(MEASURE_HEAP) && MEASURE_HEAP > 0
static void _heap_start(void) {
    _heap_before = malloc_monitor_get_usage_high_watermark();
}

static void _heap_stop(void) {
    size_t after_max = malloc_monitor_get_usage_high_watermark();
    printf("Maximum application heap usage: %d bytes\n", after_max - _heap_before);
    printf("Maximum overall heap usage: %d bytes\n", after_max);
}
#endif

static void measurements_start(void) {
#if defined(MEASURE_SYSTICKS) && MEASURE_SYSTICKS > 0
    _systick_start(SysTick_LOAD_RELOAD_Msk);
#endif

#if defined(MEASURE_HEAP) && MEASURE_HEAP > 0
    _heap_start();
#endif
}

static void measurements_stop(void) {
#if defined(MEASURE_SYSTICKS) && MEASURE_SYSTICKS > 0
    _systick_stop();
#endif

#if defined(MEASURE_HEAP) && MEASURE_HEAP > 0
    _heap_stop();
#endif
}

#endif /* MEASUREMENTS_H */