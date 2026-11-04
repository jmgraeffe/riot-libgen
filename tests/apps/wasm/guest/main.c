/*
 * Test cases for the wasm_libs module generated from the configurations in ../modules/libgen_tests.
 * Every case reports PASS or FAIL through the natives of the host application (case names are checked by
 * tests/test_apps.py). Cases mirror the ones of the JS test application (../js/main.js) where possible.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "wasm_libs_runtime.h"

#define WASM_EXPORT __attribute__((visibility("default")))

/* provided by the host application */
void test_pass(const char* name);
/* index is the number of the failed check in check_all(), or -1 */
void test_fail(const char* name, int32_t index, int64_t got, int64_t expected);

static void check(const char* name, int64_t got, int64_t expected) {
    if (got == expected) {
        test_pass(name);
    } else {
        test_fail(name, -1, got, expected);
    }
}

/* doubles are compared exactly (test values are exactly representable), failures are reported in thousandths */
static void check_double(const char* name, double got, double expected) {
    if (got == expected) {
        test_pass(name);
    } else {
        test_fail(name, -1, (int64_t) (got * 1000), (int64_t) (expected * 1000));
    }
}

static void check_all(const char* name, size_t n, const int64_t* got, const int64_t* expected) {
    for (size_t i = 0; i < n; ++i) {
        if (got[i] != expected[i]) {
            test_fail(name, (int32_t) i, got[i], expected[i]);
            return;
        }
    }
    test_pass(name);
}

#define CHECK_ALL(name, ...) do { \
        const int64_t checks[][2] = { __VA_ARGS__ }; \
        int64_t got[sizeof(checks) / sizeof(checks[0])]; \
        int64_t expected[sizeof(checks) / sizeof(checks[0])]; \
        for (size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i) { \
            got[i] = checks[i][0]; \
            expected[i] = checks[i][1]; \
        } \
        check_all(name, sizeof(checks) / sizeof(checks[0]), got, expected); \
    } while (0)

static bool streq(const char* a, const char* b) {
    while (*a != '\0' && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

/* function handles: called by the host */

WASM_EXPORT int guest_sub(int a, int b) {
    return a - b;
}

WASM_EXPORT double guest_mul(float f, double d) {
    return f * d;
}

/* strings passed to the guest are only valid during the call, so they need to be copied */
static char received_string[32];
WASM_EXPORT void guest_string_receiver(const char* s) {
    size_t i = 0;
    for (; s[i] != '\0' && i < sizeof(received_string) - 1; ++i) {
        received_string[i] = s[i];
    }
    received_string[i] = '\0';
}

static uint8_t received_data[8];
static size_t received_len = 0;
static bool received_null_data = false;
WASM_EXPORT void guest_bytes_receiver(const uint8_t* data, size_t len) {
    received_null_data = (data == NULL);
    received_len = len;
    for (size_t i = 0; i < len && i < sizeof(received_data); ++i) {
        received_data[i] = data[i];
    }
}

static size_t received_length = 0;
WASM_EXPORT void guest_string_length_receiver(const char* s) {
    while (*s++ != '\0') {
        ++received_length;
    }
}

static bool received_null_string = false;
WASM_EXPORT void guest_null_string_receiver(const char* s) {
    received_null_string = (s == NULL);
}

static int void_calls = 0;
WASM_EXPORT void guest_count(void) {
    ++void_calls;
}

static int received_array[3];
WASM_EXPORT void guest_array_receiver(int values[3]) {
    for (int i = 0; i < 3; ++i) {
        received_array[i] = values[i];
    }
}

static int correct_arrays = 0;
WASM_EXPORT void guest_array_counter(int values[3]) {
    if (values != NULL && values[0] == 11 && values[1] == 22 && values[2] == 33) {
        ++correct_arrays;
    }
}

static tests_measurement_t received_measurement;
WASM_EXPORT void guest_measurement_receiver(tests_measurement_t m) {
    received_measurement = m;
}

static int correct_measurements = 0;
WASM_EXPORT void guest_measurement_counter(tests_measurement_t m) {
    if (m.val[0] == 100 && m.val[1] == 101 && m.val[2] == 102 && m.unit == 7 && m.scale == -3) {
        ++correct_measurements;
    }
}

WASM_EXPORT tests_measurement_t* guest_measurement_sender(void) {
    static tests_measurement_t m = { .val = { 1, 2, 3 }, .unit = 4, .scale = 5 };
    return &m;
}

WASM_EXPORT int guest_counter_receiver(int64_t c) {
    return tests_counter_increment(c);
}

WASM_EXPORT int64_t guest_counter_sender(void) {
    return tests_counter_get(5);
}

WASM_EXPORT const char* guest_string_sender(void) {
    return "hello";
}

WASM_EXPORT int main(void) {
    /* primitives */

    check("primitives.int", tests_add_ints(2, 3), 5);
    check("primitives.int_negative", tests_add_ints(-7, 3), -4);
    check_double("primitives.double", tests_negate_double(1.5), -1.5);
    check_double("primitives.float", tests_halve_float(3), 1.5);
    check("primitives.int64", tests_add_int64(4000000000LL, 5), 4000000005LL);
    check("primitives.char", tests_next_char('A'), 'B');
    check("primitives.unsigned", tests_sum_unsigned(200, 60000, 4000000000U), 4000060200LL);
    check("primitives.size_t", tests_double_size(21), 42);
    check("primitives.no_parameters", tests_get_answer(), 42);
    tests_count_call();
    tests_count_call();
    check("primitives.void", tests_get_call_count(), 2);
    check("primitives.original_name", tests_renamed_function(5), 15);

    /* constants */

    check("constants.number", TESTS_ANSWER, 42);
    check("constants.string", streq(TESTS_GREETING, "hello"), true);

    /* strings */

    check("strings.length", tests_string_length("hello"), 5);
    check("strings.content", tests_string_checksum("hello"), 99162322);
    check("strings.utf8", tests_string_checksum("h\xc3\xa9llo"), 3162660225LL);
    check("strings.empty", tests_string_length(""), 0);
    check("strings.null", tests_string_is_null(NULL), 1);

    /* byte arrays */

    check("byte_arrays.content", tests_bytes_checksum((const uint8_t*) "abc", 3), 96354);
    check("byte_arrays.length_parameter", tests_bytes_checksum((const uint8_t*) "abcdef", 3), 96354);
    {
        uint8_t buf[4] = { 'x', 'x', 'x', 'x' };
        tests_fill_bytes(buf, 4);
        CHECK_ALL("byte_arrays.writable", { buf[0], 'a' }, { buf[1], 'b' }, { buf[2], 'c' }, { buf[3], 'd' });
    }
    check("byte_arrays.null", tests_bytes_is_null(NULL, 0), 1);
    /* not tested, as panics end the test run: a length parameter that exceeds the WASM memory */

    /* function handles */

    check("function_handles.int", tests_apply_int_op(10, 4, WASM_LIBS_FUNCTION_HANDLE("guest_sub")), 6);

    check_double("function_handles.float", tests_apply_float_op(1.5, 4, WASM_LIBS_FUNCTION_HANDLE("guest_mul")), 6);

    tests_call_with_string(WASM_LIBS_FUNCTION_HANDLE("guest_string_receiver"));
    check("function_handles.string_parameter", streq(received_string, "hello from host"), true);
    {
        size_t sent_length = tests_call_with_strings(WASM_LIBS_FUNCTION_HANDLE("guest_string_length_receiver"));
        check("function_handles.string_parameter_terminated", received_length, sent_length);
    }
    tests_call_with_null_string(WASM_LIBS_FUNCTION_HANDLE("guest_null_string_receiver"));
    check("function_handles.string_parameter_null", received_null_string, true);

    tests_call_with_bytes(WASM_LIBS_FUNCTION_HANDLE("guest_bytes_receiver"));
    CHECK_ALL("function_handles.byte_array_parameter",
              { received_len, 5 }, { received_data[0], 1 }, { received_data[1], 2 }, { received_data[2], 3 },
              { received_data[3], 0 }, { received_data[4], 127 });

    tests_call_with_null_bytes(WASM_LIBS_FUNCTION_HANDLE("guest_bytes_receiver"));
    CHECK_ALL("function_handles.byte_array_parameter_null", { received_null_data, true }, { received_len, 0 });

    tests_call_void_twice(WASM_LIBS_FUNCTION_HANDLE("guest_count"));
    check("function_handles.void", void_calls, 2);

    tests_call_with_array(WASM_LIBS_FUNCTION_HANDLE("guest_array_receiver"));
    CHECK_ALL("function_handles.array_parameter",
              { received_array[0], 11 }, { received_array[1], 22 }, { received_array[2], 33 });

    /* copies passed to function handles must not use up the memory of the guest */
    tests_call_with_array_repeatedly(1000, WASM_LIBS_FUNCTION_HANDLE("guest_array_counter"));
    check("function_handles.array_parameter_repeatedly", correct_arrays, 1000);

    tests_call_with_measurement(WASM_LIBS_FUNCTION_HANDLE("guest_measurement_receiver"));
    CHECK_ALL("function_handles.struct_parameter",
              { received_measurement.val[0], 100 }, { received_measurement.val[1], 101 },
              { received_measurement.val[2], 102 }, { received_measurement.unit, 7 },
              { received_measurement.scale, -3 });

    tests_call_with_measurement_repeatedly(1000, WASM_LIBS_FUNCTION_HANDLE("guest_measurement_counter"));
    check("function_handles.struct_parameter_repeatedly", correct_measurements, 1000);

    check("function_handles.struct_return",
          tests_checksum_from_measurement_callback(WASM_LIBS_FUNCTION_HANDLE("guest_measurement_sender")), 54321);

    /* WASM only: strings returned from function handles */
    check("function_handles.string_return",
          tests_wasm_length_from_string_callback(WASM_LIBS_FUNCTION_HANDLE("guest_string_sender")), 5);

    /* pointer handles */

    /* note: calls are made before CHECK_ALL, as the evaluation order of initializers is unspecified in C */
    int64_t c0 = tests_counter_get(0);
    {
        int first = tests_counter_increment(c0);
        int second = tests_counter_increment(c0);
        CHECK_ALL("pointer_handles.roundtrip", { first, 1 }, { second, 2 });
    }

    int64_t c1 = tests_counter_get(1);
    int64_t c2 = tests_counter_get(2);
    {
        int c1_first = tests_counter_increment(c1);
        int c2_first = tests_counter_increment(c2);
        int c1_second = tests_counter_increment(c1);
        CHECK_ALL("pointer_handles.distinct",
                  { c1 != c2, true }, { c1_first, 1 }, { c2_first, 1 }, { c1_second, 2 });
    }

    /* pointer handles passed to and returned from function handles */
    check("pointer_handles.function_handle_parameter",
          tests_call_with_counter(5, WASM_LIBS_FUNCTION_HANDLE("guest_counter_receiver")), 1);
    check("pointer_handles.function_handle_return",
          tests_increment_from_callback(WASM_LIBS_FUNCTION_HANDLE("guest_counter_sender")), 2);

    /* all 5 slots (max_entries) are in use from here on */
    {
        int64_t first = tests_counter_get(3);
        int64_t second = tests_counter_get(3);
        check("pointer_handles.same_pointer_same_handle", first, second);
    }

    /* handle 0 represents NULL */
    check("pointer_handles.null_to_runtime", tests_counter_null(), 0);
    check("pointer_handles.null_from_runtime", tests_counter_is_null(0), 1);

    /* released pointers are resolved to NULL, and their slots are reused */
    {
        int released_first = tests_counter_release(c0);
        int released_second = tests_counter_release(c0);
        int is_null = tests_counter_is_null(c0);
        int64_t c4 = tests_counter_get(4);
        int incremented = tests_counter_increment(c4);
        CHECK_ALL("pointer_handles.release_and_reuse",
                  { released_first, 1 }, { released_second, 0 }, { is_null, 1 }, { c4, c0 }, { incremented, 1 });
    }
    /* not tested, as panics end the test run: a full table, handles out of bounds */

    /* arrays */

    {
        int values[3] = { 1, 2, 3 };
        check("arrays.int", tests_sum_int_array(values), 6);
    }
    {
        double values[2] = { 0.5, 0.25 };
        check_double("arrays.double", tests_sum_double_array(values), 0.75);
    }

    /* structs */

    {
        tests_measurement_t m = { .val = { 1, 2, 3 }, .unit = 4, .scale = 5 };
        check("structs.parameter", tests_measurement_checksum(&m), 54321);
    }
    {
        tests_measurement_t m = { .val = { -1, 0, 1 }, .unit = 0, .scale = -1 };
        check("structs.parameter_negative", tests_measurement_checksum(&m), -9901);
    }
    {
        tests_measurement_t m = *tests_make_measurement(10);
        CHECK_ALL("structs.return", { m.val[0], 10 }, { m.val[1], 11 }, { m.val[2], 12 }, { m.unit, 7 },
                  { m.scale, -3 });
    }
    {
        tests_point_t p = { .x = 1.5, .y = 4 };
        check_double("structs.double_fields", tests_point_dot(&p), 6);
    }

    return 0;
}
