#include "net/sock/udp.h"
#include "net/sock/util.h"
#include "uri_parser.h"
#include "debug.h"

#include "easycoap.h"

#ifndef CONFIG_URI_MAX
#define CONFIG_URI_MAX      128
#endif

// from `external/RIOT/examples/gcoap/client.c`
static int _uristr2remote(const char* uri, sock_udp_ep_t* remote, const char** path,
                          char* buf, size_t buf_len)
{
    if (strlen(uri) >= buf_len) {
        DEBUG_PUTS("URI too long");
        return 1;
    }
    uri_parser_result_t urip;
    if (uri_parser_process(&urip, uri, strlen(uri))) {
        DEBUG("'%s' is not a valid URI\n", uri);
        return 1;
    }
    memcpy(buf, urip.host, urip.host_len);
    buf[urip.host_len] = '\0';
    if (urip.port_str_len) {
        strcat(buf, ":");
        strncat(buf, urip.port_str, urip.port_str_len);
        buf[urip.host_len + 1 + urip.port_str_len] = '\0';
    }
    if (sock_udp_name2ep(remote, buf) != 0) {
        DEBUG("Could not resolve address '%s'\n", buf);
        return -1;
    }
    if (remote->port == 0) {
        remote->port = !strncmp("coaps", urip.scheme, 5) ? CONFIG_GCOAPS_PORT : CONFIG_GCOAP_PORT;
    }
    if (path) {
        *path = urip.path;
    }
    strcpy(buf, uri);
    return 0;
}

// from `external/RIOT/examples/gcoap/client.c`
static gcoap_socket_type_t _get_tl(const char *uri)
{
    if (!strncmp(uri, "coaps", 5)) {
        return GCOAP_SOCKET_TYPE_DTLS;
    }
    else if (!strncmp(uri, "coap", 4)) {
        return GCOAP_SOCKET_TYPE_UDP;
    }
    return GCOAP_SOCKET_TYPE_UNDEF;
}

static void _resp_handler(const gcoap_request_memo_t* memo, coap_pkt_t* pdu,
                          const sock_udp_ep_t* remote)
{
	(void) remote;
	((easycoap_resp_handler_t) memo->context)(pdu->hdr->code, pdu->payload_len > 0 ? pdu->payload : NULL, pdu->payload_len);
}

ssize_t easycoap_request(const char* uri, coap_method_flags_t method, const uint8_t* payload, uint16_t payload_len, easycoap_resp_handler_t resp_handler) {
	sock_udp_ep_t remote;
	const char* path;
	char uri_buf[CONFIG_URI_MAX] = {0};

	coap_pkt_t pdu;
	uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];

	size_t len;

	// parse URI and resolve remote
	if (_uristr2remote(uri, &remote, &path, uri_buf, CONFIG_URI_MAX) != 0) {
		DEBUG_PUTS("Could not parse URI");
		return -EFAULT;
	}

	// prepare PDU
	gcoap_req_init(&pdu, buf, CONFIG_GCOAP_PDU_BUF_SIZE, method, NULL);

	// add path if existing
	if (path) coap_opt_add_uri_path(&pdu, path);

	// optionally add payload
	if (payload_len > 0) {
		len = coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);
		len += payload_len;
		if (len > CONFIG_GCOAP_PDU_BUF_SIZE) {
			DEBUG_PUTS("Payload does not fit into PDU buffer");
			return -ENOMEM;
		}
		memcpy(pdu.payload, payload, payload_len);
	} else {
		len = coap_opt_finish(&pdu, COAP_OPT_FINISH_NONE);
	}

	// use response handler as context so it can be called from universal response
	gcoap_socket_type_t tl = _get_tl(uri);
	return gcoap_req_send(buf, len, &remote, NULL, _resp_handler, resp_handler, tl);
}