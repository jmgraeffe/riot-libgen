function js_libs_function_handle(name) {
    return name;
}

function string_sender() {
    return "Exterminate!";
}

function string_receiver(str) {
    print("Got string from host: " + str);
}

function bytes_sender() {
    return "String in disguise!";
}

function bytes_receiver(data, len) {
    print("Got data with length " + len + " from host: " + data);
}

function array_receiver(arr) {
    var s = "";
    for (var i = 0; i < 3; ++i) {
        s += arr[i] + " ";
    }
    print("Got array with following elements from host: " + s);
}

// strings
playground.pass_string_to_host("The cake is a lie!");
//playground.pass_string_to_host_via_function_handle(js_libs_function_handle("string_sender"));
playground.pass_string_to_app_via_function_handle(js_libs_function_handle("string_receiver"));
    
// bytes
playground.pass_bytes_to_host("Actually a string!", 17);
//playground.pass_bytes_to_host_via_function_handle(js_libs_function_handle("bytes_sender"));
playground.pass_bytes_to_app_via_function_handle(js_libs_function_handle("bytes_receiver"));

// pointers
var ptr = playground.pass_pointer_to_app_via_pointer_handle();
playground.pass_pointer_to_host_via_pointer_handle(ptr);

// arrays
var arr = [1, 2, 3];
playground.pass_array_to_host(arr);
playground.pass_array_to_app_via_function_handle(js_libs_function_handle("array_receiver"));