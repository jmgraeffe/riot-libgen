#include <stdio.h>
#include <string.h>

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"
#include "js_libs.h"

/* include header generated from main.js */
#include "blob/main.js.h"

void call_void_func(void (*func)(void)) {
    func();
}

void call_sophisticated_func(int (*func)(int, int), int a, int b, const char* op) {
    printf("%d %s %d = %d\n", a, op, b, func(a, b));
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
    if (!js_libs_register_io_natives()) {
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
