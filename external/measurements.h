#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <stdio.h>
#include <math.h>

#if defined(MEASURE_CPU_CYCLES) && MEASURE_CPU_CYCLES > 0
#if defined(CPU_STM32)
#include "cpu.h"
#include "clk.h"
#elif defined(CPU_FAM_ESP32)
#include "xtensa/hal.h"
#endif
#endif

#if defined(MEASURE_HEAP) && MEASURE_HEAP > 0
#include "malloc_monitor.h"

static size_t _heap_before;
#endif

#if defined(MEASURE_CPU_CYCLES) && MEASURE_CPU_CYCLES > 0
#if defined(CPU_FAM_ESP32)
static uint32_t _cpu_cycles_before;
#endif

static void _cpu_cycles_start(void) {
#if defined(CPU_STM32)
    // disable SysTick, clear value
    SysTick->CTRL = 0;
    SysTick->VAL = 0;
    // prepare value in re-load register
    SysTick->LOAD = SysTick_LOAD_RELOAD_Msk;
    // start and wait for the load value to be applied
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    while (SysTick->VAL == 0) { /* wait for SysTick to start and spin */ }
#elif defined(CPU_FAM_ESP32)
    _cpu_cycles_before = xthal_get_ccount();
#else
    #error "Cycle counter not implemented for this platform!"
#endif
}

static void _cpu_cycles_stop(void) {
#if defined(CPU_STM32)
    uint32_t ticks = SysTick_LOAD_RELOAD_Msk - SysTick->VAL;
    //uint32_t ticks_per_sec = coreclk();
    printf("=== %ld\n", ticks);
    //double nsecs = ((double) ticks / (double) ticks_per_sec) * 1000000000.0;

    //printf("Elapsed systicks: %ld\n", ticks);
    //printf("Core clock: %d MHz\n", (int) floor(ticks_per_sec/1000000.0));
    /*printf("Calculated nanoseconds: %f\n", nsecs);
    printf("Calculated microseconds: %f\n", nsecs/1000.0);
    printf("Calculated milliseconds: %f\n", nsecs/1000000.0);*/
#elif defined(CPU_FAM_ESP32)
    uint32_t ticks = xthal_get_ccount() - _cpu_cycles_before;
    printf("=== %ld\n", ticks);
#else
    #error "Cycle counter not implemented for this platform!"
#endif
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
#if defined(MEASURE_CPU_CYCLES) && MEASURE_CPU_CYCLES > 0
    _cpu_cycles_start();
#endif

#if defined(MEASURE_HEAP) && MEASURE_HEAP > 0
    _heap_start();
#endif
}

static void measurements_stop(void) {
#if defined(MEASURE_CPU_CYCLES) && MEASURE_CPU_CYCLES > 0
    _cpu_cycles_stop();
#endif

#if defined(MEASURE_HEAP) && MEASURE_HEAP > 0
    _heap_stop();
#endif
}

#endif /* MEASUREMENTS_H */