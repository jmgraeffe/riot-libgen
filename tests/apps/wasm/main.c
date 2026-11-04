/*
 * LibGen test application for WAMR: runs guest/main.c (compiled to guest.wasm), which exercises the
 * functions generated from the configurations in ../modules/libgen_tests and prints a "PASS <case>" or
 * "FAIL <case>: <details>" line per test case. "LIBGEN_TESTS_DONE" marks the end of the test run.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wasm_export.h"
#include "wasm_libs.h"

#include "blob/guest.wasm.h"

/* 8 byte alignment is required by WAMR on some platforms, e.g. ESP32 */
#define RUNTIME_POOL_SIZE (48 * 1024)
static uint8_t runtime_pool[RUNTIME_POOL_SIZE] __attribute__((aligned(8)));

#define APP_STACK_SIZE (8 * 1024)
#define APP_HEAP_SIZE (8 * 1024)

static void print_int64(int64_t value) {
    /* printf implementations on microcontrollers often do not support 64 bit integers */
    char buffer[21];
    char* p = &buffer[sizeof(buffer) - 1];
    uint64_t abs = value < 0 ? -(uint64_t) value : (uint64_t) value;
    *p = '\0';
    do {
        *--p = '0' + (abs % 10);
        abs /= 10;
    } while (abs > 0);
    if (value < 0) {
        *--p = '-';
    }
    printf("%s", p);
}

static void test_pass(wasm_exec_env_t exec_env, const char* name) {
    (void) exec_env;
    printf("PASS %s\n", name);
}

static void test_fail(wasm_exec_env_t exec_env, const char* name, int32_t index, int64_t got, int64_t expected) {
    (void) exec_env;
    printf("FAIL %s: ", name);
    if (index >= 0) {
        printf("check %d: ", (int) index);
    }
    printf("got ");
    print_int64(got);
    printf(", expected ");
    print_int64(expected);
    printf("\n");
}

static NativeSymbol test_symbols[] = {
    { "test_pass", (void*) test_pass, "($)", NULL },
    { "test_fail", (void*) test_fail, "($iII)", NULL },
};

static int run(void) {
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(init_args));
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = runtime_pool;
    init_args.mem_alloc_option.pool.heap_size = sizeof(runtime_pool);
    if (!wasm_runtime_full_init(&init_args)) {
        puts("FAIL runtime: could not initialize WAMR");
        return 1;
    }

    if (!wasm_runtime_register_natives("env", test_symbols, sizeof(test_symbols) / sizeof(NativeSymbol))) {
        puts("FAIL registration: could not register test natives");
        return 1;
    }
    if (!wasm_libs_register_tests_natives("env")) {
        puts("FAIL registration: could not register natives of library 'tests'");
        return 1;
    }
    if (!wasm_libs_register_tests_wasm_natives("env")) {
        puts("FAIL registration: could not register natives of library 'tests_wasm'");
        return 1;
    }

    /* WAMR needs writable bytecode */
    uint8_t* bytecode = malloc(guest_wasm_len);
    if (bytecode == NULL) {
        puts("FAIL loading: out of memory");
        return 1;
    }
    memcpy(bytecode, guest_wasm, guest_wasm_len);

    char error_buf[128];
    wasm_module_t module = wasm_runtime_load(bytecode, guest_wasm_len, error_buf, sizeof(error_buf));
    if (module == NULL) {
        printf("FAIL loading: %s\n", error_buf);
        return 1;
    }

    wasm_module_inst_t module_inst = wasm_runtime_instantiate(module, APP_STACK_SIZE, APP_HEAP_SIZE,
                                                              error_buf, sizeof(error_buf));
    if (module_inst == NULL) {
        printf("FAIL instantiation: %s\n", error_buf);
        return 1;
    }

    wasm_libs_set_wasm_module_inst(module_inst);

    wasm_application_execute_main(module_inst, 0, NULL);
    const char* exception = wasm_runtime_get_exception(module_inst);
    if (exception != NULL) {
        printf("FAIL exception: %s\n", exception);
    }

    wasm_runtime_deinstantiate(module_inst);
    wasm_runtime_unload(module);
    wasm_runtime_destroy();
    free(bytecode);
    return 0;
}

int main(void) {
    run();
    puts("LIBGEN_TESTS_DONE");
    return 0;
}
