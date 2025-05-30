#include <stdio.h>
#include <string.h>

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"
#include "js_libs.h"

#include "blob/main.js.h"

#if (defined(MEASURE_SYSTICKS) && MEASURE_SYSTICKS > 0) || (defined(MEASURE_HEAP) && MEASURE_HEAP > 0)
#include "../../../external/measurements.h"
#include "periph/pm.h"
#endif

int main(void) {
#if (defined(MEASURE_SYSTICKS) && MEASURE_SYSTICKS > 0) || (defined(MEASURE_HEAP) && MEASURE_HEAP > 0)
    measurements_start();
#endif

    jerry_value_t main_app, ret_value;
    int res = 0;

    // initialize engine
    jerry_init(JERRY_INIT_EMPTY);

    // register print function
    jerryx_handler_register_global((const jerry_char_t *) "print",
                                   jerryx_handler_print);

    // register natives grouped by libraries generated by RIOT LibGen to global object
    if (!js_libs_register_easysaul_natives()) {
        printf("Could not register natives!\n");
        res = 1;
    }
    if (!js_libs_register_math_natives()) {
        printf("Could not register natives!\n");
        res = 1;
    }

    // parse and run main.js
    main_app = jerry_parse(NULL, 0, main_js, main_js_len, JERRY_PARSE_NO_OPTS);
    if (!jerry_value_is_error(main_app)) {
        ret_value = jerry_run(main_app);

        if (jerry_value_is_error(ret_value)) {
            printf("Could not run main script!\n");
            res = 1;
        }

        jerry_release_value(ret_value);
    }
    jerry_release_value(main_app);

    // cleanup engine
    jerry_cleanup();

#if (defined(MEASURE_SYSTICKS) && MEASURE_SYSTICKS > 0) || (defined(MEASURE_HEAP) && MEASURE_HEAP > 0)
    measurements_stop();
    #if defined(RESET_LOOP) && RESET_LOOP > 0
    pm_reboot();
    #endif
#endif

    return res;
}
