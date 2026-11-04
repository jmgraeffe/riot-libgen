/*
 * LibGen test application for JerryScript: runs main.js, which exercises the functions generated from
 * ../modules/libgen_tests/libgen_tests.libgen-config.yml and prints a "PASS <case>" or
 * "FAIL <case>: <details>" line per test case. "LIBGEN_TESTS_DONE" marks the end of the test run.
 */
#include <stdio.h>
#include <stdlib.h>

#include "jerryscript.h"
#include "js_libs.h"
#include "libgen_tests.h"

#include "blob/main.js.h"

static void print_value(jerry_value_t value) {
    jerry_value_t str = jerry_value_to_string(value);
    jerry_size_t size = jerry_get_string_size(str);
    char* buffer = calloc(size + 1, 1);
    if (buffer != NULL) {
        jerry_string_to_char_buffer(str, (jerry_char_t*) buffer, size);
        printf("%s", buffer);
        free(buffer);
    }
    jerry_release_value(str);
}

static jerry_value_t print_handler(const jerry_value_t func_val, const jerry_value_t this_val,
                                   const jerry_value_t argv[], const jerry_length_t argc) {
    (void) func_val;
    (void) this_val;
    for (jerry_length_t i = 0; i < argc; ++i) {
        if (i > 0) {
            printf(" ");
        }
        print_value(argv[i]);
    }
    printf("\n");
    return jerry_create_undefined();
}

static void register_print(void) {
    jerry_value_t global = jerry_get_global_object();
    jerry_value_t name = jerry_create_string((const jerry_char_t*) "print");
    jerry_value_t func = jerry_create_external_function(print_handler);
    jerry_release_value(jerry_set_property(global, name, func));
    jerry_release_value(func);
    jerry_release_value(name);
    jerry_release_value(global);
}

static void print_error(const char* prefix, jerry_value_t error) {
    jerry_value_t value = jerry_get_value_from_error(error, false);
    printf("%s: ", prefix);
    print_value(value);
    printf("\n");
    jerry_release_value(value);
}

int main(void) {
    jerry_init(JERRY_INIT_EMPTY);
    register_print();

    if (!js_libs_register_tests_natives()) {
        puts("FAIL registration: could not register natives of library 'tests'");
    }
    if (!js_libs_register_tests_js_natives()) {
        puts("FAIL registration: could not register natives of library 'tests_js'");
    }
    libgen_tests_saul_init();
    if (!js_libs_register_saul_natives()) {
        puts("FAIL registration: could not register natives of library 'saul'");
    }

    jerry_value_t parsed = jerry_parse(NULL, 0, main_js, main_js_len, JERRY_PARSE_NO_OPTS);
    if (jerry_value_is_error(parsed)) {
        print_error("FAIL parsing", parsed);
    } else {
        jerry_value_t ret = jerry_run(parsed);
        if (jerry_value_is_error(ret)) {
            print_error("FAIL uncaught exception", ret);
        }
        jerry_release_value(ret);
    }
    jerry_release_value(parsed);

    jerry_cleanup();

    puts("LIBGEN_TESTS_DONE");
    return 0;
}
