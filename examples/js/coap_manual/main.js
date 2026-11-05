// constants of the net library (set by hand, see net/af.h, net/sock.h, net/coap.h and net/nanocoap.h)
net.AF_INET6 = 4;
net.SOCK_NO_TIMEOUT = 4294967295;
net.COAP_PORT = 5683;
net.COAP_METHOD_GET = 1;
net.COAP_CODE_CONTENT = 69;
net.COAP_CODE_METHOD_NOT_ALLOWED = 133;
net.COAP_FORMAT_TEXT = 0;
net.BUFFER_SIZE = 128;

var dew_point = "42";

// receives a single request and replies with the dew point (for GET requests, regardless of the path)
function serve(sock, req, resp, pkt, remote) {
    var len = net.sock_udp_recv(sock, req, net.BUFFER_SIZE, net.SOCK_NO_TIMEOUT, remote);
    if (len < 0) return -1;
    if (net.coap_parse(pkt, req, len) < 0) return -1;

    var code = net.coap_get_code_raw(pkt) == net.COAP_METHOD_GET ? net.COAP_CODE_CONTENT
                                                                  : net.COAP_CODE_METHOD_NOT_ALLOWED;
    len = net.coap_reply_simple(pkt, code, resp, net.BUFFER_SIZE, net.COAP_FORMAT_TEXT, dew_point, dew_point.length);
    if (len < 0) return -1;

    return net.sock_udp_send(sock, resp, len, remote) < 0 ? -1 : 0;
}

// instances live on the host until they are freed
var local = net.new_sock_udp_ep({family: net.AF_INET6, netif: 0, port: net.COAP_PORT});
var remote = net.new_sock_udp_ep();
var sock = net.new_sock_udp();
var req = net.new_buffer();
var resp = net.new_buffer();
var pkt = net.new_coap_pkt();

if (net.sock_udp_create(sock, local, 0, 0) == 0) {
    if (serve(sock, req, resp, pkt, remote) < 0) {
        print("Could not serve request!");
    }
    net.sock_udp_close(sock);
}

net.free_coap_pkt(pkt);
net.free_buffer(resp);
net.free_buffer(req);
net.free_sock_udp(sock);
net.free_sock_udp_ep(remote);
net.free_sock_udp_ep(local);
