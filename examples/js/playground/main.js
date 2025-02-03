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

// strings
playground.pass_string_to_host("The cake is a lie!");
//playground.pass_string_to_host_via_function_handle(js_libs_function_handle("string_sender"));
playground.pass_string_to_app_via_function_handle(js_libs_function_handle("string_receiver"));
    
// bytes
playground.pass_bytes_to_host("Actually a string!", 17);
//playground.pass_bytes_to_host_via_function_handle(js_libs_function_handle("bytes_sender"));
playground.pass_bytes_to_app_via_function_handle(js_libs_function_handle("bytes_receiver"));