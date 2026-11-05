/*
 * Host library exposed by the LibGen test applications (tests/apps/js and tests/apps/wasm).
 *
 * Every function is designed so that the guest application can verify the result itself, e.g. by
 * returning a checksum of what the host received, so that tests only need to check PASS/FAIL lines.
 */
#ifndef LIBGEN_TESTS_H
#define LIBGEN_TESTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* layout mirrors RIOT's phydat_t */
typedef struct {
    int16_t val[3];
    uint8_t unit;
    int8_t scale;
} measurement_t;

typedef struct {
    double x;
    double y;
} point_t;

typedef struct {
    int value;
} counter_t;

typedef struct {
    bool ok;
    int16_t code;
} status_t;

/* only a and b are exposed, hidden is only known to the host */
typedef struct {
    int a;
    int hidden;
    int16_t b;
} partial_t;

typedef struct {
    counter_t* c;
    int increment;
} counter_ref_t;

/* allocatable: only sum is exposed */
typedef struct {
    int sum;
    int count;
} accumulator_t;

/* allocatable without exposed fields */
typedef struct {
    int value;
} opaque_t;

/* pointer fields (to allocatable structs and arrays) */
typedef struct {
    int factor;
    accumulator_t* acc;
} scaler_t;

typedef struct {
    const uint8_t* data;
} view_t;

/* allocatable, pointing to its own struct */
typedef struct node {
    int value;
    struct node* next;
} node_t;

/* nested struct, with fields that are aligned and sized differently in WASM and on 64 bit hosts */
typedef struct {
    uint8_t tag;
    measurement_t m;
    double weight;
    size_t count;
} envelope_t;

/* primitives */
int add_ints(int a, int b);
double negate_double(double d);
float halve_float(float f);
int64_t add_int64(int64_t a, int64_t b);
char next_char(char c);
uint32_t sum_unsigned(uint8_t a, uint16_t b, uint32_t c);
size_t double_size(size_t n);
int get_answer(void);
void count_call(void);
int get_call_count(void);
int original_function_name(int x);
bool is_even(int x);
int bool_to_int(bool b);

/* strings */
size_t string_length(const char* s);
uint32_t string_checksum(const char* s);
int string_is_null(const char* s);

/* byte arrays */
uint32_t bytes_checksum(const uint8_t* data, size_t len);
void fill_bytes(uint8_t* buf, size_t len);
int bytes_is_null(const uint8_t* data, size_t len);

/* function handles */
int apply_int_op(int a, int b, int (*op)(int a, int b));
double apply_float_op(float f, double d, double (*op)(float f, double d));
void call_with_string(void (*cb)(const char* s));
size_t call_with_strings(void (*cb)(const char* s));
void call_with_null_string(void (*cb)(const char* s));
void call_with_bytes(void (*cb)(const uint8_t* data, size_t len));
void call_with_null_bytes(void (*cb)(const uint8_t* data, size_t len));
void call_void_twice(void (*cb)(void));
size_t length_from_string_callback(const char* (*cb)(void));
int call_with_bool(bool b, int (*cb)(bool b));
bool bool_from_callback(int x, bool (*cb)(int x));

/* pointer handles */
counter_t* counter_get(int index);
int counter_increment(counter_t* c);
counter_t* counter_null(void);
int counter_is_null(counter_t* c);
int counter_release(counter_t* c);
int call_with_counter(int index, int (*cb)(counter_t* c));
int increment_from_callback(counter_t* (*cb)(void));

/* arrays */
int sum_int_array(int values[3]);
double sum_double_array(double values[2]);
void call_with_array(void (*cb)(int values[3]));
int count_true(bool values[3]);
void call_with_array_repeatedly(int times, void (*cb)(int values[3]));

/* structs */
int32_t measurement_checksum(measurement_t m);
measurement_t make_measurement(int16_t base);
void call_with_measurement(void (*cb)(measurement_t m));
void call_with_measurement_repeatedly(int times, void (*cb)(measurement_t m));
int32_t checksum_from_measurement_callback(measurement_t (*cb)(void));
double point_dot(point_t p);
int status_checksum(status_t s);
status_t make_status(bool ok);

/* output parameters */
int get_count(int* count);
void get_flags(bool* flag, double* d);
int find_counter(int index, counter_t** c);
int read_measurement(int16_t base, measurement_t* m);
void get_values(int values[3]);
size_t read_bytes(uint8_t* buf, size_t len);

/* void pointers as byte arrays */
uint32_t void_bytes_checksum(const void* data, size_t len);
void fill_void_bytes(void* buf, size_t len);
size_t read_void_bytes(void* buf, size_t len);
void call_with_void_bytes(void (*cb)(const void* data, size_t len));
int get_name(const char** name);

/* SAUL: registers a fake temperature ("fake_temp") and humidity sensor ("fake_hum") */
void libgen_tests_saul_init(void);

/* struct conversion */
int partial_checksum(partial_t p);
partial_t make_partial(int a);
int counter_ref_increment(counter_ref_t r);
counter_ref_t make_counter_ref(int index);
int32_t envelope_checksum(envelope_t e);
envelope_t make_envelope(uint8_t tag);
void read_envelope(envelope_t* e);
void call_with_envelope(void (*cb)(envelope_t e));
int32_t checksum_from_envelope_callback(envelope_t (*cb)(void));
int sum_partials(partial_t values[2]);
size_t sum_sizes(size_t values[3]);
void get_partials(partial_t values[2]);

/* allocatable structs */
void accumulator_add(accumulator_t* acc, int value);
int accumulator_sum(const accumulator_t* acc);
int accumulator_count(const accumulator_t* acc);
void accumulator_keep(accumulator_t* acc);
int kept_accumulator_sum(void);
void opaque_set(opaque_t* o, int value);
int opaque_get(const opaque_t* o);
int32_t measurement_checksum_by_pointer(const measurement_t* m);

/* named arrays */
void buffer_fill(uint8_t* buf, uint8_t value);
int buffer_sum(const uint8_t* buf);
int32_t samples_sum(const int32_t* samples);
int32_t measurements_checksum(const measurement_t* measurements);

/* pointer fields */
void scaler_apply(scaler_t s, int value);
scaler_t make_scaler(accumulator_t* acc, int factor);
int view_sum(view_t v);
int list_sum(const node_t* head);

#endif /* LIBGEN_TESTS_H */
