#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "wasm_export.h"
#include "blob/hello.wasm.h"
#include "easysaul.h"

bool iwasm_runtime_init(void);
void iwasm_runtime_destroy(void);
int iwasm_instance_exec_main(wasm_module_inst_t, int, char**);

void io_printd_native(wasm_exec_env_t exec_env, double d) {
    (void) exec_env;
    printf("%f", d);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    void (*func_ptr)(wasm_exec_env_t, double);
} io_printd_ptr = {
    .func_ptr = io_printd_native
};

double math_log_native(wasm_exec_env_t exec_env, double arg) {
    (void) exec_env;
    return log(arg);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    double (*func_ptr)(wasm_exec_env_t, double);
} math_log_ptr = {
    .func_ptr = math_log_native
};
    
double easysaul_reg_find_and_read_native(wasm_exec_env_t exec_env, uint8_t type, const char* name) {
    (void) exec_env;
    saul_reg_t* dev = saul_reg_find_type_and_name(type, name);
    return easysaul_reg_read(dev);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    double (*func_ptr)(wasm_exec_env_t, uint8_t, const char*);
} easysaul_reg_find_and_read_ptr = {
    .func_ptr = easysaul_reg_find_and_read_native
};

int main(void)
{
    NativeSymbol native_symbols[] = {
        {
            "io_printd",
            io_printd_ptr.void_ptr,
            "(F)", // void io_printd(d)
            NULL
        },
        {
            "math_log",
            math_log_ptr.void_ptr,
            "(F)F", // double math_log(arg)
            NULL
        },
        {
            "easysaul_reg_find_and_read",
            easysaul_reg_find_and_read_ptr.void_ptr,
            "(i$)F", // double easysaul_reg_find_and_read(type, name)
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
    if (!wasm_runtime_register_natives("env", native_symbols, 3)) {
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
    wasm_module_inst_t wasm_module_inst = NULL;
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