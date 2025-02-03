#include <stdio.h>
#include <string.h>

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"
#include "js_libs.h"

/* include header generated from main.js */
#include "blob/main.js.h"

void pass_string_to_host(const char* str) {
    printf("Got string from app: %s\n", str);
}

void pass_string_to_host_via_function_handle(const char* (*func)(void)) {
    printf("Got string from app: %s\n", func());
}

void pass_string_to_app_via_function_handle(void (*func)(const char*)) {
    func("Greetings from host!\n");
}

void pass_bytes_to_host(const uint8_t* data, size_t len) {
    printf("Got data with length %d from app: %s\n", len, data);
}

void pass_bytes_to_host_via_function_handle(const uint8_t* (*func)(void)) {
    printf("Got data from app: %s\n", (const char*) func());
}

void pass_bytes_to_app_via_function_handle(void (*func)(const uint8_t*, size_t)) {
    func((const uint8_t*) "Bruce Stringsteen!\n", strlen("Bruce Stringsteen!\n"));
}

int main(void) {
    jerry_value_t parsed_code, ret_value;
    int res = 0;

    /* initialize engine, no flags, default configuration */
    jerry_init(JERRY_INIT_EMPTY);

    /* Register the print function in the global object. */
    jerryx_handler_register_global((const jerry_char_t *) "print",
                                   jerryx_handler_print);

    /* register libraries to global object */
    if (!js_libs_register_playground_natives()) {
        printf("Could not register natives!\n");
        res = 1;
    }

    /* setup global scope code */
    parsed_code = jerry_parse(NULL, 0, main_js, main_js_len, JERRY_PARSE_NO_OPTS);
    if (!jerry_value_is_error(parsed_code)) {
        /* execute the parsed source code in the global scope */
        ret_value = jerry_run(parsed_code);

        if (jerry_value_is_error(ret_value)) {
            printf("js_run(): Script Error!");
            res = 1;
        }

        jerry_release_value(ret_value);
    }
    jerry_release_value(parsed_code);

    /* cleanup engine */
    jerry_cleanup();

    return res;
}
