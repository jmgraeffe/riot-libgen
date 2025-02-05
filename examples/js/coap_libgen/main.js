function js_libs_function_handle(name) {
    return name;
}

function req_handler(method, payload, len) {
    easycoap.set_resp_payload("42", 2);
    return easycoap.CODE_CONTENT;
}

function resp_handler(code, payload, payload_len) {
    print("Dew point: " + payload + " °C");
}

// start server
easycoap.serve("/dew_point", easycoap.METHOD_GET, js_libs_function_handle("req_handler"));

// send request
easycoap.request("coap://[::1]/dew_point", easycoap.METHOD_GET, null, 0, js_libs_function_handle("resp_handler"));