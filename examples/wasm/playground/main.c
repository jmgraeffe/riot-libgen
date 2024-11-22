#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "wasm_export.h"
#include "blob/hello.wasm.h"
#include "wasm_libs.h"

bool iwasm_runtime_init(void);
void iwasm_runtime_destroy(void);
int iwasm_instance_exec_main(wasm_module_inst_t, int, char**);

void call_void_func(void (*func)(void)) {
    func();
}

void call_sophisticated_func(int (*func)(int, int), int a, int b, char op) {
    printf("%d %c %d = %d\n", a, op, b, func(a, b));
}

int main(void)
{
    if (!iwasm_runtime_init()) {
        printf("Could not initialize WASM runtime!\n");
        return 1;
    }
    if (!wasm_libs_register_io_natives("env")) {
        printf("Could not register natives!\n");
        iwasm_runtime_destroy();
        return 1;
    }

    uint8_t* bytecode = malloc(hello_wasm_len);
    size_t bytecode_len = hello_wasm_len;
    memcpy(bytecode, hello_wasm, bytecode_len);

    wasm_module_t wasm_module = NULL;
    {   /* load WASM module */
        char error_buf[128];
        if (!(wasm_module = wasm_runtime_load(bytecode, bytecode_len,
                                              error_buf, sizeof(error_buf)))) {
            puts(error_buf);
            return -1;
        }
    }

    wasm_module_inst_t wasm_module_inst = NULL;
    {   /* instantiate the module */
        char error_buf[128];
        if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module, 8 * 1024,
            8 * 1024, error_buf, sizeof(error_buf)))) {
            puts(error_buf);
            return -1;
        }
    }

    //TODO comment
    wasm_libs_set_wasm_module_inst(wasm_module_inst);

    const char *exception = 0;
    int ret = 0;
    int argc = 0;
    char** argv = NULL;
    wasm_application_execute_main(wasm_module_inst, argc, argv);
    if ((exception = wasm_runtime_get_exception(wasm_module_inst))) {
        puts(exception);
        return -1;
    }
    if (argc > 0) {
        ret = *((int*)argv);
    }
    printf("ret = %d\n", ret);

    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

    /* unload WASM module */
    wasm_runtime_unload(wasm_module);

    iwasm_runtime_destroy();

    return 0;
}
