// Test cases for the js_libs module generated from ../modules/libgen_tests/libgen_tests.libgen-config.yml.
// Every case prints "PASS <case>" or "FAIL <case>: <details>" (case names are checked by tests/test_apps.py).
// Note: JerryScript is built with the minimal profile, so there are no builtins (Math, String methods, ...).

function check(name, got, expected) {
    if (got === expected) {
        print("PASS " + name);
    } else {
        print("FAIL " + name + ": got " + got + ", expected " + expected);
    }
}

function check_all(name, checks) {
    for (var i = 0; i < checks.length; ++i) {
        if (checks[i][0] !== checks[i][1]) {
            print("FAIL " + name + ": check " + i + ": got " + checks[i][0] + ", expected " + checks[i][1]);
            return;
        }
    }
    print("PASS " + name);
}

// primitives

check("primitives.int", tests.add_ints(2, 3), 5);
check("primitives.int_negative", tests.add_ints(-7, 3), -4);
check("primitives.double", tests.negate_double(1.5), -1.5);
check("primitives.float", tests.halve_float(3), 1.5);
check("primitives.int64", tests.add_int64(4000000000, 5), 4000000005);
check("primitives.char", tests.next_char(65), 66);
check("primitives.unsigned", tests.sum_unsigned(200, 60000, 4000000000), 4000060200);
check("primitives.size_t", tests.double_size(21), 42);
check("primitives.no_parameters", tests.get_answer(), 42);
var count_call_result = tests.count_call();
tests.count_call();
check_all("primitives.void", [
    [typeof count_call_result, "undefined"],
    [tests.get_call_count(), 2]
]);
check("primitives.original_name", tests.renamed_function(5), 15);

// constants

check("constants.number", tests.ANSWER, 42);
check("constants.string", tests.GREETING, "hello");

// strings

check("strings.length", tests.string_length("hello"), 5);
check("strings.content", tests.string_checksum("hello"), 99162322);
check("strings.utf8", tests.string_checksum("héllo"), 3162660225);
check("strings.empty", tests.string_length(""), 0);
check("strings.null", tests.string_is_null(null), 1);

// byte arrays (represented as strings in JS)

check("byte_arrays.content", tests.bytes_checksum("abc", 3), 96354);
check("byte_arrays.length_parameter", tests.bytes_checksum("abcdef", 3), 96354);
check("byte_arrays.writable", typeof tests.fill_bytes("xxxx", 4), "undefined");
// BUG: the length parameter is not checked against the actual length of the data (heap buffer over-read)
// check("byte_arrays.length_too_large", tests.bytes_checksum("ab", 5), ...);

// function handles

function js_sub(a, b) {
    return a - b;
}
check("function_handles.int", tests.apply_int_op(10, 4, "js_sub"), 6);

var received_string = null;
function js_string_receiver(s) {
    received_string = s;
}
tests.call_with_string("js_string_receiver");
check("function_handles.string_parameter", received_string, "hello from host");
var received_length = 0;
function js_string_length_receiver(s) {
    received_length += s.length;
}
check("function_handles.string_parameter_terminated",
      tests.call_with_strings("js_string_length_receiver"), received_length);

var received_data = null;
var received_len = null;
function js_bytes_receiver(data, len) {
    received_data = data;
    received_len = len;
}
tests.call_with_bytes("js_bytes_receiver");
check_all("function_handles.byte_array_parameter", [
    [received_len, 5],
    [received_data, "\x01\x02\x03\x00\x7f"]
]);

var void_calls = 0;
function js_count() {
    ++void_calls;
}
tests.call_void_twice("js_count");
check("function_handles.void", void_calls, 2);

var received_array = null;
function js_array_receiver(values) {
    received_array = values;
}
tests.call_with_array("js_array_receiver");
check_all("function_handles.array_parameter", [
    [received_array[0], 11],
    [received_array[1], 22],
    [received_array[2], 33]
]);

var received_measurement = null;
function js_measurement_receiver(m) {
    received_measurement = m;
}
tests.call_with_measurement("js_measurement_receiver");
check_all("function_handles.struct_parameter", [
    [received_measurement.val[0], 100],
    [received_measurement.val[1], 101],
    [received_measurement.val[2], 102],
    [received_measurement.unit, 7],
    [received_measurement.scale, -3]
]);

function js_measurement_sender() {
    return {val: [1, 2, 3], unit: 4, scale: 5};
}
check("function_handles.struct_return", tests.checksum_from_measurement_callback("js_measurement_sender"), 54321);

// pointer handles

var c0 = tests.counter_get(0);
check_all("pointer_handles.roundtrip", [
    [tests.counter_increment(c0), 1],
    [tests.counter_increment(c0), 2]
]);

var c1 = tests.counter_get(1);
var c2 = tests.counter_get(2);
check_all("pointer_handles.distinct", [
    [c1 !== c2, true],
    [tests.counter_increment(c1), 1],
    [tests.counter_increment(c2), 1],
    [tests.counter_increment(c1), 2]
]);

// all 4 slots (max_entries) are in use from here on
check("pointer_handles.same_pointer_same_handle", tests.counter_get(3), tests.counter_get(3));

// handle 0 represents NULL
check("pointer_handles.null_to_runtime", tests.counter_null(), 0);
check("pointer_handles.null_from_runtime", tests.counter_is_null(0), 1);

// released pointers are resolved to NULL, and their slots are reused
var c4 = null;
check_all("pointer_handles.release_and_reuse", [
    [tests.counter_release(c0), 1],
    [tests.counter_release(c0), 0],
    [tests.counter_is_null(c0), 1],
    [(c4 = tests.counter_get(4)), c0],
    [tests.counter_increment(c4), 1]
]);
// not tested, as panics end the test run: a full table, handles out of bounds

// arrays

check("arrays.int", tests.sum_int_array([1, 2, 3]), 6);
check("arrays.double", tests.sum_double_array([0.5, 0.25]), 0.75);

// structs

check("structs.parameter", tests.measurement_checksum({val: [1, 2, 3], unit: 4, scale: 5}), 54321);
check("structs.parameter_negative", tests.measurement_checksum({val: [-1, 0, 1], unit: 0, scale: -1}), -9901);
var m = tests.make_measurement(10);
check_all("structs.return", [
    [m.val[0], 10],
    [m.val[1], 11],
    [m.val[2], 12],
    [m.unit, 7],
    [m.scale, -3]
]);
check("structs.double_fields", tests.point_dot({x: 1.5, y: 4}), 6);
