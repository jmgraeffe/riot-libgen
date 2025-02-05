var EASYCOAP_CODE_CONTENT = 69;
var EASYCOAP_METHOD_GET = 1;

function req_handler(method, payload, len) {
    easycoap.set_resp_payload("42", 2);
    return EASYCOAP_CODE_CONTENT;
}

function resp_handler(code, payload, payload_len) {
    print("Dew point: " + payload + " °C");
}

// start server
easycoap.serve("/dew_point", EASYCOAP_METHOD_GET);

// send request
easycoap.request("coap://[::1]/dew_point", EASYCOAP_METHOD_GET, null, 0);