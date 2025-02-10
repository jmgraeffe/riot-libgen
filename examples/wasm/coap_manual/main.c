#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <panic.h>

#include "wasm_export.h"
#include "blob/hello.wasm.h"
#include "easycoap.h"

bool iwasm_runtime_init(void);
void iwasm_runtime_destroy(void);
int iwasm_instance_exec_main(wasm_module_inst_t, int, char**);

static wasm_module_inst_t wasm_module_inst = NULL;

static uint8_t req_handler(uint16_t method, const uint8_t* payload, uint16_t payload_len) {
    // get environment
    wasm_exec_env_t wasm_exec_env = wasm_runtime_get_exec_env_singleton(wasm_module_inst);

    // lookup function handle in WASM runtime namespace
    wasm_function_inst_t wasm_function_inst = wasm_runtime_lookup_function(wasm_module_inst, "req_handler");

    // construct parameters for upcoming function call
    size_t num_args = 3;
    wasm_val_t args[3];
    args[0].kind = WASM_I32;
    args[0].of.i32 = (int32_t) method;
    args[1].kind = WASM_I32;
    args[1].of.i32 = (int32_t) wasm_runtime_module_dup_data(wasm_module_inst, (const char*) payload, payload_len);
    args[2].kind = WASM_I32;
    args[2].of.i32 = (int32_t) payload_len;

    // make space for return value
    size_t num_results = 1;
    wasm_val_t results[1];

    // call function handle
    if (!wasm_runtime_call_wasm_a(wasm_exec_env, wasm_function_inst, num_results, results, num_args, args)) {
        const char* exception = 0;
        if ((exception = wasm_runtime_get_exception(wasm_module_inst))) {
            core_panic(PANIC_GENERAL_ERROR, exception);
        }
    }

    // cleanup
    wasm_runtime_module_free(wasm_module_inst, (uint64_t) args[1].of.i32);
    
    return (uint8_t) results[0].of.i32;
}
    
static void resp_handler(uint8_t code, const uint8_t* payload, uint16_t payload_len) {
    // get environment
    wasm_exec_env_t wasm_exec_env = wasm_runtime_get_exec_env_singleton(wasm_module_inst);

    // lookup function handle in WASM runtime namespace
    wasm_function_inst_t wasm_function_inst = wasm_runtime_lookup_function(wasm_module_inst, "resp_handler");

    // construct parameters for upcoming function call
    size_t num_args = 3;
    wasm_val_t args[3];
    args[0].kind = WASM_I32;
    args[0].of.i32 = (int32_t) code;
    args[1].kind = WASM_I32;
    args[1].of.i32 = (int32_t) wasm_runtime_module_dup_data(wasm_module_inst, (const char*) payload, payload_len);
    args[2].kind = WASM_I32;
    args[2].of.i32 = (int32_t) payload_len;

    // make space for return value
    size_t num_results = 0;
    wasm_val_t results[1];

    // call function handle
    if (!wasm_runtime_call_wasm_a(wasm_exec_env, wasm_function_inst, num_results, results, num_args, args)) {
        const char* exception = 0;
        if ((exception = wasm_runtime_get_exception(wasm_module_inst))) {
            core_panic(PANIC_GENERAL_ERROR, exception);
        }
    }

    // cleanup
    wasm_runtime_module_free(wasm_module_inst, (uint64_t) args[1].of.i32);
}
    
int easycoap_serve_native(wasm_exec_env_t exec_env, const char* path, uint16_t methods) {
    (void) exec_env;

    // call native function
    return easycoap_serve(path, methods, req_handler);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int (*func_ptr)(wasm_exec_env_t, const char*, uint16_t);
} easycoap_serve_ptr = {
    .func_ptr = easycoap_serve_native
};
    
void easycoap_set_resp_payload_native(wasm_exec_env_t exec_env, const uint8_t* payload, uint16_t payload_len) {
    (void) exec_env;

    // prepare function parameters
    payload = (const uint8_t*) wasm_runtime_addr_app_to_native(wasm_module_inst, (uintptr_t) payload);

    // call native function
    easycoap_set_resp_payload(payload, payload_len);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    void (*func_ptr)(wasm_exec_env_t, const uint8_t*, uint16_t);
} easycoap_set_resp_payload_ptr = {
    .func_ptr = easycoap_set_resp_payload_native
};
    
ssize_t easycoap_request_native(wasm_exec_env_t exec_env, const char* uri, uint16_t method, const uint8_t* payload, uint16_t payload_len) {
    (void) exec_env;

    // prepare function parameters
    payload = (const uint8_t*) wasm_runtime_addr_app_to_native(wasm_module_inst, (uintptr_t) payload);
    
    // call native function
    ssize_t res = easycoap_request(uri, method, payload, payload_len, resp_handler);

    return res;
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    ssize_t (*func_ptr)(wasm_exec_env_t, const char*, uint16_t, const uint8_t*, uint16_t);
} easycoap_request_ptr = {
    .func_ptr = easycoap_request_native
};

void printsl_native(wasm_exec_env_t exec_env, const char* s, size_t l) {
    (void) exec_env;
    printf("%.*s", l, s);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    void (*func_ptr)(wasm_exec_env_t, const char* s, size_t l);
} printsl_ptr = {
    .func_ptr = printsl_native
};

int main(void)
{
    NativeSymbol native_symbols[] = {
        {
            "easycoap_serve",
            easycoap_serve_ptr.void_ptr,
            "($i)i", // int easycoap_serve(path, methods)
            NULL
        },
        {
            "easycoap_set_resp_payload",
            easycoap_set_resp_payload_ptr.void_ptr,
            "(ii)", // void easycoap_set_resp_payload(payload, payload_len)
            NULL
        },
        {
            "easycoap_request",
            easycoap_request_ptr.void_ptr,
            "($iii)I", // ssize_t easycoap_request(uri, method, payload, payload_len)
            NULL
        },
        {
            "printsl",
            printsl_ptr.void_ptr,
            "($i)", // void printsl(const char* s, size_t l)
            NULL
        }
    };

    uint8_t* bytecode = malloc(hello_wasm_len);
    size_t bytecode_len = hello_wasm_len;
    memcpy(bytecode, hello_wasm, bytecode_len);

    if (!iwasm_runtime_init()) {
        printf("Could not initialize WASM runtime!\n");
        return 1;
    }

    // register natives
    if (!wasm_runtime_register_natives("env", native_symbols, 4)) {
        printf("Could not register natives!\n");
        return 0;
    }

    // load WASM module (the compiled WASM binary)
    wasm_module_t wasm_module = NULL;
    {
        char error_buf[128];
        if (!(wasm_module = wasm_runtime_load(bytecode, bytecode_len,
                                              error_buf, sizeof(error_buf)))) {
            puts(error_buf);
            return 1;
        }
    }

    // instantiate WASM module
    {
        char error_buf[128];
        if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module, 8 * 1024,
            8 * 1024, error_buf, sizeof(error_buf)))) {
            puts(error_buf);
            return 1;
        }
    }

    // execute main function of the loaded WASM module
    const char *exception = 0;
    //int ret = 0;
    int argc = 0;
    char** argv = NULL;
    wasm_application_execute_main(wasm_module_inst, argc, argv);
    if ((exception = wasm_runtime_get_exception(wasm_module_inst))) {
        puts(exception);
        return 1;
    }
    /*if (argc > 0) {
        ret = *((int*)argv);
    }
    printf("ret = %d\n", ret);*/

    // destroy the WASM module instance
    wasm_runtime_deinstantiate(wasm_module_inst);

    // unload WASM module
    wasm_runtime_unload(wasm_module);

    // destroy WASM runtime
    iwasm_runtime_destroy();

    return 0;
}