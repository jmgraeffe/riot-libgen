#include "net/gcoap.h"

#define EASYCOAP_METHOD_GET (1)

#define	EASYCOAP_CODE_CONTENT (69)

typedef uint8_t (*easycoap_req_handler_t)(uint16_t method, const uint8_t* payload, uint16_t payload_len);
typedef void (*easycoap_resp_handler_t)(uint8_t code, const uint8_t* payload, uint16_t payload_len);

/* server */
int easycoap_serve(const char* path, coap_method_flags_t methods, easycoap_req_handler_t req_handler);
void easycoap_set_resp_payload(const uint8_t* payload, uint16_t payload_len);

/* client */
ssize_t easycoap_request(const char* uri, coap_method_flags_t method, const uint8_t* payload, uint16_t payload_len, easycoap_resp_handler_t resp_handler);