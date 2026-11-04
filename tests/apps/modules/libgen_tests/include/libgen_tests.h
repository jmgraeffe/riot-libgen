/*
 * Host library exposed by the LibGen test applications (tests/apps/js and tests/apps/wasm).
 *
 * Every function is designed so that the guest application can verify the result itself, e.g. by
 * returning a checksum of what the host received, so that tests only need to check PASS/FAIL lines.
 */
#ifndef LIBGEN_TESTS_H
#define LIBGEN_TESTS_H

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

/* strings */
size_t string_length(const char* s);
uint32_t string_checksum(const char* s);
int string_is_null(const char* s);

/* byte arrays */
uint32_t bytes_checksum(const uint8_t* data, size_t len);
void fill_bytes(uint8_t* buf, size_t len);

/* function handles */
int apply_int_op(int a, int b, int (*op)(int a, int b));
void call_with_string(void (*cb)(const char* s));
size_t call_with_strings(void (*cb)(const char* s));
void call_with_null_string(void (*cb)(const char* s));
void call_with_bytes(void (*cb)(const uint8_t* data, size_t len));
void call_void_twice(void (*cb)(void));
size_t length_from_string_callback(const char* (*cb)(void));

/* pointer handles */
counter_t* counter_get(int index);
int counter_increment(counter_t* c);
counter_t* counter_null(void);
int counter_is_null(counter_t* c);
int counter_release(counter_t* c);

/* arrays */
int sum_int_array(int values[3]);
double sum_double_array(double values[2]);
void call_with_array(void (*cb)(int values[3]));

/* structs */
int32_t measurement_checksum(measurement_t m);
measurement_t make_measurement(int16_t base);
void call_with_measurement(void (*cb)(measurement_t m));
int32_t checksum_from_measurement_callback(measurement_t (*cb)(void));
double point_dot(point_t p);

#endif /* LIBGEN_TESTS_H */
