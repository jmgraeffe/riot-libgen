#include "debug.h"

#include "easycoap.h"

static uint8_t _payload[EASYCOAP_MAX_PAYLOAD_SIZE];
static uint16_t _payload_len;

static ssize_t _req_handler(coap_pkt_t *pdu, uint8_t* buf, size_t len, coap_request_ctx_t *ctx) {
	size_t resp_len;

	// reset response
	_payload_len = 0;

	// call handler
	((easycoap_req_handler_t) ctx->resource->context)(pdu->hdr->code, pdu->payload, pdu->payload_len);

	// initialize response in request PDU
	gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
	
	if (_payload_len > 0) {
		resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);
		resp_len += _payload_len;
		if (len > CONFIG_GCOAP_PDU_BUF_SIZE) {
			DEBUG_PUTS("Payload does not fit into PDU buffer");
			return -ENOMEM;
		}
		memcpy(pdu->payload, _payload, _payload_len);
	} else {
		resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_NONE);
	}

	return resp_len;
}

void easycoap_set_resp_payload(const uint8_t* payload, uint16_t payload_len) {
	if (payload_len > EASYCOAP_MAX_PAYLOAD_SIZE) {
		DEBUG_PUTS("Response payload too large, truncating");
		payload_len = EASYCOAP_MAX_PAYLOAD_SIZE;
	}
	memcpy(_payload, payload, payload_len);
	_payload_len = payload_len;
}

int easycoap_serve(const char* path, coap_method_flags_t methods, easycoap_req_handler_t req_handler) {
	coap_resource_t* resource = calloc(1, sizeof(coap_resource_t));
	if (!resource) return -ENOMEM;
	resource->path = malloc(strlen(path));
	if (!resource->path) return -ENOMEM;
	strcpy((char*) resource->path, path);
	resource->methods = methods;
	resource->handler = _req_handler;
	// use context to store function pointer
	resource->context = req_handler;

	gcoap_listener_t* listener = calloc(1, sizeof(gcoap_listener_t));
	if (!listener) return -ENOMEM;
	listener->resources = resource;
	listener->resources_len = 1;
	listener->tl_type = GCOAP_SOCKET_TYPE_UNDEF;

	gcoap_register_listener(listener);
	return 0;
}