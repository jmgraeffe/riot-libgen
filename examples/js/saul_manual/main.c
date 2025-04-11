#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"
#include "easysaul.h"
#include "blob/main.js.h"
#include "panic.h"

#if (defined(MEASURE_SYSTICKS) && MEASURE_SYSTICKS > 0) || (defined(MEASURE_HEAP) && MEASURE_HEAP > 0)
#include "../../../external/measurements.h"
#include "periph/pm.h"
#endif

static jerry_value_t math_log_native(const jerry_value_t func_val, const jerry_value_t this_val, const jerry_value_t argv[], const jerry_length_t argc) {
    (void) func_val;
    (void) this_val;
    (void) argc;

    return jerry_create_number(log((double) jerry_get_number_value(argv[0])));
}

static jerry_value_t easysaul_reg_find_and_read_native(const jerry_value_t func_val, const jerry_value_t this_val, const jerry_value_t argv[], const jerry_length_t argc) {
    (void) func_val;
    (void) this_val;
    (void) argc;

    // prepare function parameters
    uint8_t type = (uint8_t) jerry_get_number_value(argv[0]);
    jerry_value_t value_name = jerry_value_to_string(argv[1]);
    size_t length_name = jerry_get_string_size(value_name);
    char* name = (char*) calloc(length_name + 1, sizeof(char));
    if (name == NULL) {
        core_panic(PANIC_GENERAL_ERROR, "could not allocate memory");
    }
    jerry_string_to_char_buffer(value_name, (jerry_char_t*) name, length_name);
    jerry_release_value(value_name);
    int8_t power = (int8_t) jerry_get_number_value(argv[2]);

    // call native function
    saul_reg_t* dev = saul_reg_find_type_and_name(type, name);

    // cleanup
    free(name);
    printf("power = %d\n", power);
    return jerry_create_number(easysaul_reg_read(dev, power));
}

static jerry_value_t empty_constructor(const jerry_value_t func_val, const jerry_value_t this_val, const jerry_value_t argv[], const jerry_length_t argc) {
    (void) func_val;
    (void) this_val;
    (void) argv;
    (void) argc;

    return jerry_create_undefined();
}

int register_math_natives(void) {
    jerry_value_t constructor = jerry_create_external_function(empty_constructor);
    jerry_value_t library_object = jerry_construct_object(constructor, NULL, 0);
    
    // export natives and add them to library object
    {
        jerry_value_t property_name_function = jerry_create_string((const jerry_char_t*) ("log"));
        jerry_value_t property_value_function = jerry_create_external_function(math_log_native);
        jerry_value_t res = jerry_set_property(library_object, property_name_function, property_value_function);
        if (jerry_value_is_error(res)) {
            jerry_release_value(property_name_function);
            jerry_release_value(property_value_function);
            jerry_release_value(res);
            return 0;
        }

        jerry_release_value(property_name_function);
        jerry_release_value(property_value_function);
        jerry_release_value(res);
    }

    // put library as property into global object
    jerry_value_t global_object = jerry_get_global_object();
    jerry_value_t property_name_library = jerry_create_string((const jerry_char_t*) ("math"));
    jerry_value_t res = jerry_set_property(global_object, property_name_library, library_object);
    if (jerry_value_is_error(res)) {
        jerry_release_value(constructor);
        jerry_release_value(library_object);
        jerry_release_value(global_object);
        jerry_release_value(property_name_library);
        jerry_release_value(res);
        return 0;
    }

    // cleanup
    jerry_release_value(constructor);
    jerry_release_value(library_object);
    jerry_release_value(global_object);
    jerry_release_value(property_name_library);
    jerry_release_value(res);

    return 1;
}

static int register_easysaul_natives(void) {
    jerry_value_t constructor = jerry_create_external_function(empty_constructor);
    jerry_value_t library_object = jerry_construct_object(constructor, NULL, 0);
    
    // export natives and add them to library object
    {
        jerry_value_t property_name_function = jerry_create_string((const jerry_char_t*) ("reg_find_and_read"));
        jerry_value_t property_value_function = jerry_create_external_function(easysaul_reg_find_and_read_native);
        jerry_value_t res = jerry_set_property(library_object, property_name_function, property_value_function);
        if (jerry_value_is_error(res)) {
            jerry_release_value(property_name_function);
            jerry_release_value(property_value_function);
            jerry_release_value(res);
            return 0;
        }

        jerry_release_value(property_name_function);
        jerry_release_value(property_value_function);
        jerry_release_value(res);
    }

    // put library as property into global object
    jerry_value_t global_object = jerry_get_global_object();
    jerry_value_t property_name_library = jerry_create_string((const jerry_char_t*) ("easysaul"));
    jerry_value_t res = jerry_set_property(global_object, property_name_library, library_object);
    if (jerry_value_is_error(res)) {
        jerry_release_value(constructor);
        jerry_release_value(library_object);
        jerry_release_value(global_object);
        jerry_release_value(property_name_library);
        jerry_release_value(res);
        return 0;
    }

    // cleanup
    jerry_release_value(constructor);
    jerry_release_value(library_object);
    jerry_release_value(global_object);
    jerry_release_value(property_name_library);
    jerry_release_value(res);

    return 1;
}

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

    // register natives
    if (!register_math_natives()) {
        printf("Could not register natives!\n");
        res = 1;
    }
    if (!register_easysaul_natives()) {
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
