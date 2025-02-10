#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <panic.h>

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"
#include "easycoap.h"

#include "blob/main.js.h"

static uint8_t req_handler(uint16_t method, const uint8_t* payload, uint16_t payload_len) {
    // lookup function in engine context
    jerry_value_t global_object = jerry_get_global_object();
    jerry_value_t name = jerry_create_string((const jerry_char_t *) "req_handler");
    jerry_value_t func = jerry_get_property(global_object, name);

    // construct parameters for upcoming call
    jerry_value_t args[3];
    args[0] = jerry_create_number(method);
    args[1] = jerry_create_string_sz((const jerry_char_t*) payload, payload_len);
    args[2] = jerry_create_number(payload_len);

    // call JS function in engine & extract result
    jerry_value_t res_value = jerry_call_function(func, global_object, args, 3);
    uint8_t res = (uint8_t) jerry_get_number_value(res_value);

    // cleanup
    jerry_release_value(global_object);
    jerry_release_value(name);
    jerry_release_value(func);
    jerry_release_value(args[0]);
    jerry_release_value(args[1]);
    jerry_release_value(args[2]);
    jerry_release_value(res_value);

    return res;
}

static void resp_handler(uint8_t code, const uint8_t* payload, uint16_t payload_len) {
    // lookup function in engine context
    jerry_value_t global_object = jerry_get_global_object();
    jerry_value_t name = jerry_create_string((const jerry_char_t *) "resp_handler");
    jerry_value_t func = jerry_get_property(global_object, name);

    // construct parameters for upcoming call
    jerry_value_t args[3];
    args[0] = jerry_create_number(code);
    args[1] = jerry_create_string_sz((const jerry_char_t*) payload, payload_len);
    args[2] = jerry_create_number(payload_len);

    // call JS function in engine
    jerry_value_t res_value = jerry_call_function(func, global_object, args, 3);

    // cleanup
    jerry_release_value(global_object);
    jerry_release_value(name);
    jerry_release_value(func);
    jerry_release_value(args[0]);
    jerry_release_value(args[1]);
    jerry_release_value(args[2]);
    jerry_release_value(res_value);
}

static jerry_value_t easycoap_serve_native(const jerry_value_t func_val, const jerry_value_t this_val, const jerry_value_t argv[], const jerry_length_t argc) {
    (void) func_val;
    (void) this_val;
    (void) argc;

    // prepare function parameters
    char* path = NULL;
    jerry_value_t value_path = jerry_value_to_string(argv[0]);
    size_t length_path = jerry_get_string_size(value_path);
    path = (char*) calloc(length_path + 1, sizeof(char));
    if (path == NULL) {
        core_panic(PANIC_GENERAL_ERROR, "Could not allocate memory!");
    }
    jerry_string_to_char_buffer(value_path, (jerry_char_t*) path, length_path);
    jerry_release_value(value_path);
    uint16_t methods = (uint16_t) jerry_get_number_value(argv[1]);

    // call native function
    int res = easycoap_serve((const char*) path, methods, req_handler);

    // cleanup
    free(path);

    return jerry_create_number(res);
}

static jerry_value_t easycoap_set_resp_payload_native(const jerry_value_t func_val, const jerry_value_t this_val, const jerry_value_t argv[], const jerry_length_t argc
) {
    (void) func_val;
    (void) this_val;
    (void) argc;

    // prepare function parameters
    char* payload = NULL;
    jerry_value_t value_payload = jerry_value_to_string(argv[0]);
    size_t length_payload = jerry_get_string_size(value_payload);
    payload = (char*) calloc(length_payload + 1, sizeof(char));
    if (payload == NULL) {
        core_panic(PANIC_GENERAL_ERROR, "Could not allocate memory!");
    }
    jerry_string_to_char_buffer(value_payload, (jerry_char_t*) payload, length_payload);
    jerry_release_value(value_payload);
    uint16_t payload_len = (uint16_t) jerry_get_number_value(argv[1]);

    // call native function
    easycoap_set_resp_payload((const uint8_t*) payload, payload_len);

    // cleanup
    free(payload);

    return jerry_create_undefined();
}

static jerry_value_t easycoap_request_native(const jerry_value_t func_val, const jerry_value_t this_val, const jerry_value_t argv[], const jerry_length_t argc) {
    (void) func_val;
    (void) this_val;
    (void) argc;

    // prepare function parameters
    char* uri = NULL;
    jerry_value_t value_uri = jerry_value_to_string(argv[0]);
    size_t length_uri = jerry_get_string_size(value_uri);
    uri = (char*) calloc(length_uri + 1, sizeof(char));
    if (uri == NULL) {
        core_panic(PANIC_GENERAL_ERROR, "Could not allocate memory!");
    }
    jerry_string_to_char_buffer(value_uri, (jerry_char_t*) uri, length_uri);
    jerry_release_value(value_uri);
    uint16_t method = (uint16_t) jerry_get_number_value(argv[1]);
    char* payload = NULL;
    jerry_value_t value_payload = jerry_value_to_string(argv[2]);
    size_t length_payload = jerry_get_string_size(value_payload);
    payload = (char*) calloc(length_payload + 1, sizeof(char));
    jerry_string_to_char_buffer(value_payload, (jerry_char_t*) payload, length_payload);
    jerry_release_value(value_payload);
    uint16_t payload_len = (uint16_t) jerry_get_number_value(argv[3]);

    // call native function
    ssize_t res = easycoap_request((const char*) uri, method, (const uint8_t*) payload, payload_len, resp_handler);

    // cleanup
    free(uri);
    free(payload);

    return jerry_create_number(res);
}

static jerry_value_t empty_constructor(const jerry_value_t func_val, const jerry_value_t this_val, const jerry_value_t argv[], const jerry_length_t argc) {
    (void) func_val;
    (void) this_val;
    (void) argv;
    (void) argc;

    return jerry_create_undefined();
}

static int register_natives(void) {
    jerry_value_t constructor = jerry_create_external_function(empty_constructor);
    jerry_value_t library_object = jerry_construct_object(constructor, NULL, 0);
    
    // export natives and add them to library object
    {
        jerry_value_t property_name_function = jerry_create_string((const jerry_char_t*) ("serve"));
        jerry_value_t property_value_function = jerry_create_external_function(easycoap_serve_native);
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
    {
        jerry_value_t property_name_function = jerry_create_string((const jerry_char_t*) ("set_resp_payload"));
        jerry_value_t property_value_function = jerry_create_external_function(easycoap_set_resp_payload_native);
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
    {
        jerry_value_t property_name_function = jerry_create_string((const jerry_char_t*) ("request"));
        jerry_value_t property_value_function = jerry_create_external_function(easycoap_request_native);
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
    jerry_value_t property_name_library = jerry_create_string((const jerry_char_t*) ("easycoap"));
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

    return 3;
}

int main(void) {
    jerry_value_t main_app, ret_value;
    int res = 0;

    // initialize engine
    jerry_init(JERRY_INIT_EMPTY);

    // register print function
    jerryx_handler_register_global((const jerry_char_t *) "print",
                                   jerryx_handler_print);

    // register natives
    if (!register_natives()) {
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

    return res;
}
