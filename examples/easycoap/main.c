#include <stdio.h>
#include <string.h>

#include "xtimer.h"
#include "easycoap.h"

static uint8_t _echo_handler(uint16_t method, const uint8_t* payload, uint16_t payload_len) {
    if (payload_len == 0 || method != COAP_GET) {
        return COAP_CODE_BAD_REQUEST;
    }

    printf("Echoing '%.*s'!\n", payload_len, payload);

    easycoap_set_resp_payload(payload, payload_len);
    return COAP_CODE_CONTENT;
}

static void _resp_handler(uint8_t code, const uint8_t* payload, uint16_t payload_len) {
    if (payload_len == 0 || code != COAP_CODE_CONTENT) return;
    printf("Got echo '%.*s'!\n", payload_len, payload);
}

int main(void) {
    /* CoAP server */

    // serve single resource in another thread
    easycoap_serve("/echo", COAP_GET, _echo_handler);
    
    /* CoAP client */

    // retrieve the only IPv6 address
    netif_t* iface = netif_iter(NULL);
    ipv6_addr_t addr;
    if (netif_get_ipv6(iface, &addr, 1) < 0) {
        printf("Failed to get IPv6 address!\n");
        return 1;
    }
    char addr_str[IPV6_ADDR_MAX_STR_LEN];
    if (!ipv6_addr_to_str(addr_str, &addr, IPV6_ADDR_MAX_STR_LEN)) {
        printf("Failed to convert IPv6 address to string!\n");
        return 1;
    }

    // construct URI
    char uri[IPV6_ADDR_MAX_STR_LEN*2];
    sprintf(uri, "coap://[%s]/echo", addr_str);

    // send request
    xtimer_sleep(1);
    easycoap_request(uri, COAP_GET, (uint8_t*) "Hello World", strlen("Hello World"), _resp_handler);

    return 0;
}
