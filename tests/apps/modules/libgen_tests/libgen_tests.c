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

bool is_even(int x) {
    return x % 2 == 0;
}

int bool_to_int(bool b) {
    return b ? 1 : 0;
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

int bytes_is_null(const uint8_t* data, size_t len) {
    (void) len;
    return data == NULL;
}

/* function handles */

int apply_int_op(int a, int b, int (*op)(int a, int b)) {
    return op(a, b);
}

double apply_float_op(float f, double d, double (*op)(float f, double d)) {
    return op(f, d);
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

void call_with_null_string(void (*cb)(const char* s)) {
    cb(NULL);
}

void call_with_bytes(void (*cb)(const uint8_t* data, size_t len)) {
    static const uint8_t data[] = { 1, 2, 3, 0, 127 };
    cb(data, sizeof(data));
}

void call_with_null_bytes(void (*cb)(const uint8_t* data, size_t len)) {
    /* e.g. like gcoap does for responses without payload */
    cb(NULL, 0);
}

void call_void_twice(void (*cb)(void)) {
    cb();
    cb();
}

int call_with_bool(bool b, int (*cb)(bool b)) {
    return cb(b);
}

bool bool_from_callback(int x, bool (*cb)(int x)) {
    return cb(x);
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

int call_with_counter(int index, int (*cb)(counter_t* c)) {
    return cb(counter_get(index));
}

int increment_from_callback(counter_t* (*cb)(void)) {
    return counter_increment(cb());
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

void call_with_array_repeatedly(int times, void (*cb)(int values[3])) {
    for (int i = 0; i < times; ++i) {
        call_with_array(cb);
    }
}

int count_true(bool values[3]) {
    return values[0] + values[1] + values[2];
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

void call_with_measurement_repeatedly(int times, void (*cb)(measurement_t m)) {
    for (int i = 0; i < times; ++i) {
        call_with_measurement(cb);
    }
}

int32_t checksum_from_measurement_callback(measurement_t (*cb)(void)) {
    return measurement_checksum(cb());
}

double point_dot(point_t p) {
    return p.x * p.y;
}

int status_checksum(status_t s) {
    return (s.ok ? 100 : 0) + s.code;
}

status_t make_status(bool ok) {
    status_t s = { .ok = ok, .code = 7 };
    return s;
}

/* output parameters */

int get_count(int* count) {
    /* output parameters are optional, the runtime can pass NULL */
    if (count != NULL) {
        *count = 42;
    }
    return 1;
}

void get_flags(bool* flag, double* d) {
    *flag = true;
    *d = 2.5;
}

int find_counter(int index, counter_t** c) {
    *c = counter_get(index);
    return 1;
}

int read_measurement(int16_t base, measurement_t* m) {
    *m = make_measurement(base);
    return 3;
}

void get_values(int values[3]) {
    values[0] = 7;
    values[1] = 8;
    values[2] = 9;
}

size_t read_bytes(uint8_t* buf, size_t len) {
    fill_bytes(buf, len);
    return len;
}

int get_name(const char** name) {
    *name = "riot";
    return 1;
}

/* struct conversion */

int partial_checksum(partial_t p) {
    return p.a + 10 * p.b + 100 * p.hidden;
}

partial_t make_partial(int a) {
    partial_t p = { .a = a, .hidden = 99, .b = (int16_t) (a + 1) };
    return p;
}

int counter_ref_increment(counter_ref_t r) {
    r.c->value += r.increment;
    return r.c->value;
}

counter_ref_t make_counter_ref(int index) {
    counter_ref_t r = { .c = counter_get(index), .increment = 2 };
    return r;
}

int32_t envelope_checksum(envelope_t e) {
    return measurement_checksum(e.m) + 100000 * e.tag + 1000000 * (int32_t) (e.weight * 4) + 100000000 * (int32_t) e.count;
}

envelope_t make_envelope(uint8_t tag) {
    envelope_t e = { .tag = tag, .m = make_measurement(10), .weight = 2.5, .count = 7 };
    return e;
}

void read_envelope(envelope_t* e) {
    *e = make_envelope(4);
}

void call_with_envelope(void (*cb)(envelope_t e)) {
    cb(make_envelope(5));
}

int32_t checksum_from_envelope_callback(envelope_t (*cb)(void)) {
    return envelope_checksum(cb());
}

int sum_partials(partial_t values[2]) {
    return partial_checksum(values[0]) + partial_checksum(values[1]);
}

size_t sum_sizes(size_t values[3]) {
    return values[0] + values[1] + values[2];
}

void get_partials(partial_t values[2]) {
    values[0] = make_partial(1);
    values[1] = make_partial(3);
}

/* allocatable structs */

void accumulator_add(accumulator_t* acc, int value) {
    acc->sum += value;
    acc->count++;
}

int accumulator_sum(const accumulator_t* acc) {
    return acc->sum;
}

int accumulator_count(const accumulator_t* acc) {
    return acc->count;
}

/* like RIOT keeping a pointer, e.g. gcoap_register_listener() */
static accumulator_t* _kept_accumulator = NULL;

void accumulator_keep(accumulator_t* acc) {
    _kept_accumulator = acc;
}

int kept_accumulator_sum(void) {
    return _kept_accumulator != NULL ? _kept_accumulator->sum : -1;
}

void opaque_set(opaque_t* o, int value) {
    o->value = value;
}

int opaque_get(const opaque_t* o) {
    return o->value;
}

int32_t measurement_checksum_by_pointer(const measurement_t* m) {
    return measurement_checksum(*m);
}

/* named arrays */

void buffer_fill(uint8_t* buf, uint8_t value) {
    for (int i = 0; i < 4; ++i) {
        buf[i] = value;
    }
}

int buffer_sum(const uint8_t* buf) {
    int sum = 0;
    for (int i = 0; i < 4; ++i) {
        sum += buf[i];
    }
    return sum;
}

int32_t samples_sum(const int32_t* samples) {
    return samples[0] + samples[1] + samples[2];
}

int32_t measurements_checksum(const measurement_t* measurements) {
    return measurement_checksum(measurements[0]) + measurement_checksum(measurements[1]);
}

/* pointer fields */

void scaler_apply(scaler_t s, int value) {
    accumulator_add(s.acc, s.factor * value);
}

scaler_t make_scaler(accumulator_t* acc, int factor) {
    scaler_t s = { .factor = factor, .acc = acc };
    return s;
}

int view_sum(view_t v) {
    return buffer_sum(v.data);
}

int list_sum(const node_t* head) {
    int sum = 0;
    for (const node_t* n = head; n != NULL; n = n->next) {
        sum += n->value;
    }
    return sum;
}

/* void pointers as byte arrays */

uint32_t void_bytes_checksum(const void* data, size_t len) {
    return bytes_checksum((const uint8_t*) data, len);
}

void fill_void_bytes(void* buf, size_t len) {
    fill_bytes((uint8_t*) buf, len);
}

size_t read_void_bytes(void* buf, size_t len) {
    return read_bytes((uint8_t*) buf, len);
}

void call_with_void_bytes(void (*cb)(const void* data, size_t len)) {
    static const uint8_t data[] = { 4, 5, 6 };
    cb(data, sizeof(data));
}
