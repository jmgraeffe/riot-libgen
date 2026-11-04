#include <string.h>

#include "libgen_tests.h"

/* for the release functions of pointer handles */
#if defined(MODULE_JS_LIBS)
#include "js_libs.h"
#define RELEASE_COUNTER(c) js_libs_release_tests_counter_pointer_handle(c)
#elif defined(MODULE_WASM_LIBS)
#include "wasm_libs.h"
#define RELEASE_COUNTER(c) wasm_libs_release_tests_counter_pointer_handle(c)
#endif

/* primitives */

int add_ints(int a, int b) {
    return a + b;
}

double negate_double(double d) {
    return -d;
}

float halve_float(float f) {
    return f / 2;
}

int64_t add_int64(int64_t a, int64_t b) {
    return a + b;
}

char next_char(char c) {
    return c + 1;
}

uint32_t sum_unsigned(uint8_t a, uint16_t b, uint32_t c) {
    return a + b + c;
}

size_t double_size(size_t n) {
    return 2 * n;
}

int get_answer(void) {
    return 42;
}

static int _call_count = 0;

void count_call(void) {
    ++_call_count;
}

int get_call_count(void) {
    return _call_count;
}

int original_function_name(int x) {
    return x * 3;
}

/* strings */

size_t string_length(const char* s) {
    return strlen(s);
}

uint32_t string_checksum(const char* s) {
    uint32_t sum = 0;
    for (size_t i = 0; s[i] != '\0'; ++i) {
        sum = sum * 31 + (uint8_t) s[i];
    }
    return sum;
}

int string_is_null(const char* s) {
    return s == NULL;
}

/* byte arrays */

uint32_t bytes_checksum(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum = sum * 31 + data[i];
    }
    return sum;
}

void fill_bytes(uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        buf[i] = (uint8_t) ('a' + i);
    }
}

/* function handles */

int apply_int_op(int a, int b, int (*op)(int a, int b)) {
    return op(a, b);
}

void call_with_string(void (*cb)(const char* s)) {
    cb("hello from host");
}

size_t call_with_strings(void (*cb)(const char* s)) {
    /* strings of decreasing length, so that leftovers of a previous string would become visible if the
     * terminating '\0' was missing (e.g. when memory is reused, and allocations happen to have no padding) */
    static const char text[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    char buffer[sizeof(text)];
    size_t total = 0;
    for (size_t len = sizeof(text) - 1; len > 0; --len) {
        memcpy(buffer, text, len);
        buffer[len] = '\0';
        cb(buffer);
        total += len;
    }
    return total;
}

void call_with_bytes(void (*cb)(const uint8_t* data, size_t len)) {
    static const uint8_t data[] = { 1, 2, 3, 0, 127 };
    cb(data, sizeof(data));
}

void call_void_twice(void (*cb)(void)) {
    cb();
    cb();
}

size_t length_from_string_callback(const char* (*cb)(void)) {
    return strlen(cb());
}

/* pointer handles */

static counter_t _counters[8];

counter_t* counter_get(int index) {
    return &_counters[index];
}

int counter_increment(counter_t* c) {
    return ++c->value;
}

counter_t* counter_null(void) {
    return NULL;
}

int counter_is_null(counter_t* c) {
    return c == NULL;
}

int counter_release(counter_t* c) {
    return RELEASE_COUNTER(c);
}

/* arrays */

int sum_int_array(int values[3]) {
    return values[0] + values[1] + values[2];
}

double sum_double_array(double values[2]) {
    return values[0] + values[1];
}

void call_with_array(void (*cb)(int values[3])) {
    int values[3] = { 11, 22, 33 };
    cb(values);
}

/* structs */

int32_t measurement_checksum(measurement_t m) {
    return m.val[0] + 10 * m.val[1] + 100 * m.val[2] + 1000 * m.unit + 10000 * m.scale;
}

measurement_t make_measurement(int16_t base) {
    measurement_t m = {
        .val = { base, (int16_t) (base + 1), (int16_t) (base + 2) },
        .unit = 7,
        .scale = -3,
    };
    return m;
}

void call_with_measurement(void (*cb)(measurement_t m)) {
    cb(make_measurement(100));
}

int32_t checksum_from_measurement_callback(measurement_t (*cb)(void)) {
    return measurement_checksum(cb());
}

double point_dot(point_t p) {
    return p.x * p.y;
}
