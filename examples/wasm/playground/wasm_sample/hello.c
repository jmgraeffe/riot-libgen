#ifdef __cplusplus
extern "C" int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default"))) extern "C"
#else
extern int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default")))
#endif

#include <stdint.h>
#include <stddef.h>

#include "../external_modules/wasm_libs/include/wasm_libs_runtime.h"

WASM_EXPORT const char* string_sender(void) {
    return "Exterminate!";
}

WASM_EXPORT void string_receiver(const char* str) {
    easyio_prints("Got string from host: ");
    easyio_puts(str);
}

WASM_EXPORT const uint8_t* bytes_sender(void) {
    return (const uint8_t*) "String in disguise!";
}

WASM_EXPORT void bytes_receiver(const uint8_t* data, size_t len) {
    easyio_prints("Got data with length ");
    easyio_printi(len);
    easyio_prints(" from host: ");
    easyio_puts((const char*) data);
}

WASM_EXPORT void array_receiver(int arr[3]) {
    easyio_prints("Got array with following elements from host: ");
    for (int i = 0; i < 3; ++i) {
        easyio_printi(arr[i]);
        easyio_prints(" ");
    }
    easyio_prints("\n");
}

WASM_EXPORT playground_phydat_t* struct_sender() {
    static playground_phydat_t data = {
        .val = {111, 222, 333},
        .unit = 42,
        .scale = 17
    };
    return &data;
}

WASM_EXPORT void struct_receiver(playground_phydat_t s) {
    easyio_prints("Got struct with following fields from host: val ");
    easyio_printi(s.val[0]);
    easyio_prints(" ");
    easyio_printi(s.val[1]);
    easyio_prints(" ");
    easyio_printi(s.val[2]);
    easyio_prints(", unit ");
    easyio_printi(s.unit);
    easyio_prints(", scale ");
    easyio_printi(s.scale);
    easyio_prints("\n");
}

WASM_EXPORT int main(int argc, char **argv)
{
    // strings
    playground_pass_string_to_host("The cake is a lie!");
    playground_pass_string_to_host_via_function_handle(WASM_LIBS_FUNCTION_HANDLE("string_sender"));
    playground_pass_string_to_app_via_function_handle(WASM_LIBS_FUNCTION_HANDLE("string_receiver"));
    
    // bytes
    playground_pass_bytes_to_host((const uint8_t*) "Actually a string!", 17);
    playground_pass_bytes_to_host_via_function_handle(WASM_LIBS_FUNCTION_HANDLE("bytes_sender"));
    playground_pass_bytes_to_app_via_function_handle(WASM_LIBS_FUNCTION_HANDLE("bytes_receiver"));
    
    // pointers
    ssize_t ptr = playground_pass_pointer_to_app_via_pointer_handle();
    playground_pass_pointer_to_host_via_pointer_handle(ptr);

    // arrays
    int arr[3] = {1, 2, 3};
    playground_pass_array_to_host(arr);
    playground_pass_array_to_app_via_function_handle(WASM_LIBS_FUNCTION_HANDLE("array_receiver"));

    // structs
    playground_phydat_t data = {
        .val = {111, 222, 333},
        .unit = 42,
        .scale = 17
    };
    playground_pass_struct_to_host(&data);
    data = *playground_pass_struct_to_app_via_return();
    struct_receiver(data);
    playground_pass_struct_to_app_via_function_handle(WASM_LIBS_FUNCTION_HANDLE("struct_receiver"));
    playground_pass_struct_to_host_via_function_handle(WASM_LIBS_FUNCTION_HANDLE("struct_sender"));

    return 0;
}