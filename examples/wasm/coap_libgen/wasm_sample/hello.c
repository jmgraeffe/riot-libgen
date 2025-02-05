#ifdef __cplusplus
extern "C" int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default"))) extern "C"
#else
extern int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default")))
#endif

#include <stddef.h>
#include <stdint.h>

// no sys/types.h present in libc implementation of embedded WAMR, just using a shortcut here
#define ssize_t intmax_t

#include "../external_modules/wasm_libs/include/wasm_libs_runtime.h"

WASM_EXPORT uint8_t req_handler(uint16_t method, const uint8_t payload, uint16_t payload_len) {
    easycoap_set_resp_payload((const uint8_t*) "42", 2);
    return EASYCOAP_CODE_CONTENT;
}

WASM_EXPORT void resp_handler(uint8_t code, const uint8_t* payload, uint16_t payload_len) {
    printf("Dew point: ");
    io_printsl((const char*) payload, 2);
    printf(" °C\n");
}

WASM_EXPORT int main(int argc, char **argv) {
    // start server
    easycoap_serve("/dew_point", EASYCOAP_METHOD_GET, WASM_LIBS_FUNCTION_HANDLE("req_handler"));

    // send request
    easycoap_request("coap://[::1]/dew_point", EASYCOAP_METHOD_GET, NULL, 0, WASM_LIBS_FUNCTION_HANDLE("resp_handler"));

    return 0;
}