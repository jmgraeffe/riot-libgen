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
check_all("primitives.bool_return", [
    [tests.is_even(4), true],
    [tests.is_even(3), false]
]);
check_all("primitives.bool_parameter", [
    [tests.bool_to_int(true), 1],
    [tests.bool_to_int(false), 0],
    // other values are converted by their truthiness
    [tests.bool_to_int(1), 1],
    [tests.bool_to_int(0), 0]
]);

// constants

check("constants.number", tests.ANSWER, 42);
check("constants.float", tests.HALF, 0.5);
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
check("byte_arrays.null", tests.bytes_is_null(null, 0), 1);
// not tested, as panics end the test run: a length parameter larger than the byte array

// function handles

function js_sub(a, b) {
    return a - b;
}
check("function_handles.int", tests.apply_int_op(10, 4, "js_sub"), 6);

function js_mul(f, d) {
    return f * d;
}
check("function_handles.float", tests.apply_float_op(1.5, 4, "js_mul"), 6);

function js_bool_receiver(b) {
    return b === true ? 10 : (b === false ? 20 : 30);
}
function js_bool_sender(x) {
    return x > 5;
}
check_all("function_handles.bool", [
    [tests.call_with_bool(true, "js_bool_receiver"), 10],
    [tests.call_with_bool(false, "js_bool_receiver"), 20],
    [tests.bool_from_callback(7, "js_bool_sender"), true],
    [tests.bool_from_callback(3, "js_bool_sender"), false]
]);

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

var received_null_string = false;
function js_null_string_receiver(s) {
    received_null_string = (s === null);
}
tests.call_with_null_string("js_null_string_receiver");
check("function_handles.string_parameter_null", received_null_string, true);

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

tests.call_with_null_bytes("js_bytes_receiver");
check_all("function_handles.byte_array_parameter_null", [
    [received_len, 0],
    [received_data, null]
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

// copies passed to function handles must not use up memory
var correct_arrays = 0;
function js_array_counter(values) {
    if (values[0] === 11 && values[1] === 22 && values[2] === 33) {
        ++correct_arrays;
    }
}
tests.call_with_array_repeatedly(1000, "js_array_counter");
check("function_handles.array_parameter_repeatedly", correct_arrays, 1000);

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

var correct_measurements = 0;
function js_measurement_counter(m) {
    if (m.val[0] === 100 && m.val[1] === 101 && m.val[2] === 102 && m.unit === 7 && m.scale === -3) {
        ++correct_measurements;
    }
}
tests.call_with_measurement_repeatedly(1000, "js_measurement_counter");
check("function_handles.struct_parameter_repeatedly", correct_measurements, 1000);

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

// pointer handles passed to and returned from function handles
function js_counter_receiver(c) {
    return tests.counter_increment(c);
}
function js_counter_sender() {
    return tests.counter_get(5);
}
check("pointer_handles.function_handle_parameter", tests.call_with_counter(5, "js_counter_receiver"), 1);
check("pointer_handles.function_handle_return", tests.increment_from_callback("js_counter_sender"), 2);

// all 5 slots (max_entries) are in use from here on
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
check("arrays.bool", tests.count_true([true, false, true]), 2);

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
var status = tests.make_status(true);
check_all("structs.bool_field", [
    [tests.status_checksum({ok: true, code: 5}), 105],
    [tests.status_checksum({ok: false, code: 5}), 5],
    [status.ok, true],
    [status.code, 7]
]);

// output parameters (primitives, pointer handles, strings and byte arrays need a box, results are in .value)

var count = {};
check_all("outputs.primitive", [
    [tests.get_count(count), 1],
    [count.value, 42]
]);

var flag = {};
var d = {};
tests.get_flags(flag, d);
check_all("outputs.bool_and_double", [
    [flag.value, true],
    [d.value, 2.5]
]);

var found = {};
check_all("outputs.pointer_handle", [
    [tests.find_counter(5, found), 1],
    [found.value, tests.counter_get(5)]
]);

var measurement = {};
check_all("outputs.struct", [
    [tests.read_measurement(10, measurement), 3],
    [measurement.val[0], 10],
    [measurement.val[1], 11],
    [measurement.val[2], 12],
    [measurement.unit, 7],
    [measurement.scale, -3]
]);

var values = [];
tests.get_values(values);
check_all("outputs.array", [
    [values[0], 7],
    [values[1], 8],
    [values[2], 9]
]);

var bytes = {};
check_all("outputs.byte_array", [
    [tests.read_bytes(bytes, 4), 4],
    [bytes.value, "abcd"]
]);

// null is passed as NULL, for optional output parameters
check("outputs.null", tests.get_count(null), 1);

var name = {};
check_all("outputs.string", [
    [tests_js.get_name(name), 1],
    [name.value, "riot"]
]);

// SAUL registry (configs/saul_reg.libgen-config.yml), with fake devices registered by the host (libgen_tests_saul.c)

var temp = saul.reg_find_type_and_name(saul.SENSE_TEMP, "fake_temp");
var hum = saul.reg_find_name("fake_hum");
check_all("saul.find", [
    [temp !== 0, true],
    [hum !== 0, true],
    [temp !== hum, true],
    [saul.reg_find_nth(0), temp],
    [saul.reg_find_nth(1), hum],
    [saul.reg_find_nth(2), 0],
    [saul.reg_find_type(saul.SENSE_HUM), hum],
    [saul.reg_find_type_and_name(saul.SENSE_HUM, "fake_temp"), 0]
]);

var res = {};
check_all("saul.read", [
    [saul.reg_read(temp, res), 1],
    [res.val[0], 2150],
    [res.val[1], 0],
    [res.val[2], 0],
    [res.unit, saul.UNIT_TEMP_C],
    [res.scale, -2]
]);
