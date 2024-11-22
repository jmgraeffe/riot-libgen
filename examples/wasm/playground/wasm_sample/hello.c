#ifdef __cplusplus
extern "C" int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default"))) extern "C"
#else
extern int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default")))
#endif

#include "../external_modules/wasm_libs/include/wasm_libs_runtime.h"

WASM_EXPORT void hello_world(void) {
    puts("Hello World!");
}

WASM_EXPORT int add(int a, int b) {
    return a + b;
}

WASM_EXPORT int sub(int a, int b) {
    return a - b;
}

WASM_EXPORT int main(int argc, char **argv)
{
    call_void_func(WASM_LIBS_FUNCTION_HANDLE("hello_world"));
    call_sophisticated_func(WASM_LIBS_FUNCTION_HANDLE("add"), 2, 3, '+');
    call_sophisticated_func(WASM_LIBS_FUNCTION_HANDLE("sub"), 42, 1337, '-');
    return 0;
}
