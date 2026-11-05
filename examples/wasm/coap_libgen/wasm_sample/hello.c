#ifdef __cplusplus
extern "C" int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default"))) extern "C"
#else
extern int printf( const char *, ...);
#define WASM_EXPORT __attribute__((visibility("default")))
#endif

#include <stddef.h>
#include <stdint.h>

#include "../external_modules/wasm_libs/include/wasm_libs_runtime.h"

static const char dew_point[] = "42";

// receives a single request and replies with the dew point (for GET requests, regardless of the path)
static int serve(int64_t sock, int64_t req, int64_t resp, int64_t pkt, int64_t remote) {
    ssize_t len = net_sock_udp_recv(sock, req, NET_BUFFER_SIZE, NET_SOCK_NO_TIMEOUT, remote);
    if (len < 0) return -1;
    if (net_coap_parse(pkt, req, len) < 0) return -1;

    uint8_t code = net_coap_get_code_raw(pkt) == NET_COAP_METHOD_GET ? NET_COAP_CODE_CONTENT
                                                                       : NET_COAP_CODE_METHOD_NOT_ALLOWED;
    len = net_coap_reply_simple(pkt, code, resp, NET_BUFFER_SIZE, NET_COAP_FORMAT_TEXT,
                                dew_point, sizeof(dew_point) - 1);
    if (len < 0) return -1;

    return net_sock_udp_send(sock, resp, len, remote) < 0 ? -1 : 0;
}

WASM_EXPORT int main(int argc, char **argv) {
    // instances live on the host until they are freed
    net_sock_udp_ep_t local_init = { .family = NET_AF_INET6, .port = NET_COAP_PORT };
    int64_t local = net_new_sock_udp_ep(&local_init);
    int64_t remote = net_new_sock_udp_ep(NULL);
    int64_t sock = net_new_sock_udp(NULL);
    int64_t req = net_new_buffer(NULL);
    int64_t resp = net_new_buffer(NULL);
    int64_t pkt = net_new_coap_pkt(NULL);

    int res = 1;
    if (net_sock_udp_create(sock, local, 0, 0) == 0) {
        res = serve(sock, req, resp, pkt, remote) == 0 ? 0 : 1;
        net_sock_udp_close(sock);
    }

    net_free_coap_pkt(pkt);
    net_free_buffer(resp);
    net_free_buffer(req);
    net_free_sock_udp(sock);
    net_free_sock_udp_ep(remote);
    net_free_sock_udp_ep(local);
    return res;
}
