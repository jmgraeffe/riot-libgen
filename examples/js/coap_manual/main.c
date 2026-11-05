#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"
#include "mutex.h"
#include "thread.h"
#include "net/ipv6/addr.h"
#include "net/nanocoap.h"
#include "net/nanocoap_sock.h"
#include "net/sock/udp.h"

#include "blob/main.js.h"

#if (defined(MEASURE_CPU_CYCLES) && MEASURE_CPU_CYCLES > 0) || (defined(MEASURE_HEAP) && MEASURE_HEAP > 0)
#include "../../../external/measurements.h"
#endif
#if defined(RESET_LOOP) && RESET_LOOP > 0
#include "periph/pm.h"
#endif

#define BUFFER_SIZE (128)
#define MAX_INSTANCES (2)

// instances live in these tables, and are passed to JS as their index + 1, so that 0 means NULL
static sock_udp_t socks[MAX_INSTANCES];
static bool socks_used[MAX_INSTANCES];
static sock_udp_ep_t eps[MAX_INSTANCES];
static bool eps_used[MAX_INSTANCES];
static uint8_t buffers[MAX_INSTANCES][BUFFER_SIZE];
static bool buffers_used[MAX_INSTANCES];
static coap_pkt_t pkts[MAX_INSTANCES];
static bool pkts_used[MAX_INSTANCES];

static int64_t add_instance(bool* used) {
    for (int64_t i = 0; i < MAX_INSTANCES; ++i) {
        if (!used[i]) {
            used[i] = true;
            return i + 1;
        }
    }
    return 0;
}

static bool is_instance(const bool* used, int64_t handle) {
    return handle > 0 && handle <= MAX_INSTANCES && used[handle - 1];
}

static bool free_instance(bool* used, int64_t handle) {
    if (!is_instance(used, handle)) {
        return false;
    }
    used[handle - 1] = false;
    return true;
}

// number of argument i, or 0 if it is missing or not a number
static double arg(const jerry_value_t argv[], const jerry_length_t argc, jerry_length_t i) {
    return i < argc && jerry_value_is_number(argv[i]) ? jerry_get_number_value(argv[i]) : 0;
}

// number property of an object, or 0 if it is missing or not a number
static double get_property(jerry_value_t object, const char* name) {
    jerry_value_t property_name = jerry_create_string((const jerry_char_t*) name);
    jerry_value_t value = jerry_get_property(object, property_name);
    double number = jerry_value_is_number(value) ? jerry_get_number_value(value) : 0;
    jerry_release_value(value);
    jerry_release_value(property_name);
    return number;
}

#define NATIVE(name) static jerry_value_t name(const jerry_value_t func_val, const jerry_value_t this_val, \
                                               const jerry_value_t argv[], const jerry_length_t argc)

NATIVE(net_new_sock_udp_ep_native) {
    (void) func_val;
    (void) this_val;
    int64_t handle = add_instance(eps_used);
    if (handle > 0) {
        sock_udp_ep_t* ep = &eps[handle - 1];
        memset(ep, 0, sizeof(*ep));
        if (argc > 0 && jerry_value_is_object(argv[0])) {
            ep->family = (int) get_property(argv[0], "family");
            ep->netif = (uint16_t) get_property(argv[0], "netif");
            ep->port = (uint16_t) get_property(argv[0], "port");
        }
    }
    return jerry_create_number(handle);
}

NATIVE(net_free_sock_udp_ep_native) {
    (void) func_val;
    (void) this_val;
    return jerry_create_boolean(free_instance(eps_used, arg(argv, argc, 0)));
}

// no initial values supported
NATIVE(net_new_sock_udp_native) {
    (void) func_val;
    (void) this_val;
    (void) argv;
    (void) argc;
    return jerry_create_number(add_instance(socks_used));
}

NATIVE(net_free_sock_udp_native) {
    (void) func_val;
    (void) this_val;
    return jerry_create_boolean(free_instance(socks_used, arg(argv, argc, 0)));
}

// no initial values supported
NATIVE(net_new_buffer_native) {
    (void) func_val;
    (void) this_val;
    (void) argv;
    (void) argc;
    int64_t handle = add_instance(buffers_used);
    if (handle > 0) {
        memset(buffers[handle - 1], 0, BUFFER_SIZE);
    }
    return jerry_create_number(handle);
}

NATIVE(net_free_buffer_native) {
    (void) func_val;
    (void) this_val;
    return jerry_create_boolean(free_instance(buffers_used, arg(argv, argc, 0)));
}

// no initial values supported
NATIVE(net_new_coap_pkt_native) {
    (void) func_val;
    (void) this_val;
    (void) argv;
    (void) argc;
    return jerry_create_number(add_instance(pkts_used));
}

NATIVE(net_free_coap_pkt_native) {
    (void) func_val;
    (void) this_val;
    return jerry_create_boolean(free_instance(pkts_used, arg(argv, argc, 0)));
}

NATIVE(net_sock_udp_create_native) {
    (void) func_val;
    (void) this_val;
    int64_t sock = arg(argv, argc, 0);
    int64_t local = arg(argv, argc, 1);
    int64_t remote = arg(argv, argc, 2);
    if (!is_instance(socks_used, sock) || (local && !is_instance(eps_used, local)) || (remote && !is_instance(eps_used, remote))) {
        return jerry_create_number(-EINVAL);
    }
    return jerry_create_number(sock_udp_create(&socks[sock - 1], local ? &eps[local - 1] : NULL,
                                               remote ? &eps[remote - 1] : NULL, (uint16_t) arg(argv, argc, 3)));
}

NATIVE(net_sock_udp_close_native) {
    (void) func_val;
    (void) this_val;
    int64_t sock = arg(argv, argc, 0);
    if (is_instance(socks_used, sock)) {
        sock_udp_close(&socks[sock - 1]);
    }
    return jerry_create_undefined();
}

NATIVE(net_sock_udp_recv_native) {
    (void) func_val;
    (void) this_val;
    int64_t sock = arg(argv, argc, 0);
    int64_t data = arg(argv, argc, 1);
    size_t max_len = arg(argv, argc, 2);
    int64_t remote = arg(argv, argc, 4);
    if (!is_instance(socks_used, sock) || !is_instance(buffers_used, data) || max_len > BUFFER_SIZE || (remote && !is_instance(eps_used, remote))) {
        return jerry_create_number(-EINVAL);
    }
    return jerry_create_number(sock_udp_recv(&socks[sock - 1], buffers[data - 1], max_len, (uint32_t) arg(argv, argc, 3),
                                             remote ? &eps[remote - 1] : NULL));
}

NATIVE(net_sock_udp_send_native) {
    (void) func_val;
    (void) this_val;
    int64_t sock = arg(argv, argc, 0);
    int64_t data = arg(argv, argc, 1);
    size_t len = arg(argv, argc, 2);
    int64_t remote = arg(argv, argc, 3);
    if (!is_instance(socks_used, sock) || !is_instance(buffers_used, data) || len > BUFFER_SIZE || (remote && !is_instance(eps_used, remote))) {
        return jerry_create_number(-EINVAL);
    }
    return jerry_create_number(sock_udp_send(&socks[sock - 1], buffers[data - 1], len, remote ? &eps[remote - 1] : NULL));
}

NATIVE(net_coap_parse_native) {
    (void) func_val;
    (void) this_val;
    int64_t pkt = arg(argv, argc, 0);
    int64_t buf = arg(argv, argc, 1);
    size_t len = arg(argv, argc, 2);
    if (!is_instance(pkts_used, pkt) || !is_instance(buffers_used, buf) || len > BUFFER_SIZE) {
        return jerry_create_number(-EINVAL);
    }
    return jerry_create_number(coap_parse(&pkts[pkt - 1], buffers[buf - 1], len));
}

NATIVE(net_coap_get_code_raw_native) {
    (void) func_val;
    (void) this_val;
    int64_t pkt = arg(argv, argc, 0);
    return jerry_create_number(is_instance(pkts_used, pkt) ? coap_get_code_raw(&pkts[pkt - 1]) : 0);
}

NATIVE(net_coap_reply_simple_native) {
    (void) func_val;
    (void) this_val;
    int64_t pkt = arg(argv, argc, 0);
    int64_t buf = arg(argv, argc, 2);
    size_t len = arg(argv, argc, 3);
    if (!is_instance(pkts_used, pkt) || !is_instance(buffers_used, buf) || len > BUFFER_SIZE || argc < 7 || !jerry_value_is_string(argv[5])) {
        return jerry_create_number(-EINVAL);
    }

    // the payload is passed as string
    char payload[BUFFER_SIZE];
    size_t payload_len = arg(argv, argc, 6);
    if (payload_len > jerry_get_string_size(argv[5]) || payload_len > sizeof(payload)) {
        return jerry_create_number(-EINVAL);
    }
    jerry_string_to_char_buffer(argv[5], (jerry_char_t*) payload, payload_len);

    return jerry_create_number(coap_reply_simple(&pkts[pkt - 1], (uint8_t) arg(argv, argc, 1), buffers[buf - 1], len,
                                                 (uint16_t) arg(argv, argc, 4), payload, payload_len));
}

static jerry_value_t empty_constructor(const jerry_value_t func_val, const jerry_value_t this_val, const jerry_value_t argv[], const jerry_length_t argc) {
    (void) func_val;
    (void) this_val;
    (void) argv;
    (void) argc;

    return jerry_create_undefined();
}

static int register_function(jerry_value_t object, const char* name, jerry_external_handler_t handler) {
    jerry_value_t property_name_function = jerry_create_string((const jerry_char_t*) name);
    jerry_value_t property_value_function = jerry_create_external_function(handler);
    jerry_value_t res = jerry_set_property(object, property_name_function, property_value_function);
    int ok = !jerry_value_is_error(res);

    jerry_release_value(property_name_function);
    jerry_release_value(property_value_function);
    jerry_release_value(res);
    return ok;
}

static int register_natives(void) {
    jerry_value_t constructor = jerry_create_external_function(empty_constructor);
    jerry_value_t library_object = jerry_construct_object(constructor, NULL, 0);

    // export natives and add them to library object
    if (!register_function(library_object, "new_sock_udp_ep", net_new_sock_udp_ep_native)
        || !register_function(library_object, "free_sock_udp_ep", net_free_sock_udp_ep_native)
        || !register_function(library_object, "new_sock_udp", net_new_sock_udp_native)
        || !register_function(library_object, "free_sock_udp", net_free_sock_udp_native)
        || !register_function(library_object, "new_buffer", net_new_buffer_native)
        || !register_function(library_object, "free_buffer", net_free_buffer_native)
        || !register_function(library_object, "new_coap_pkt", net_new_coap_pkt_native)
        || !register_function(library_object, "free_coap_pkt", net_free_coap_pkt_native)
        || !register_function(library_object, "sock_udp_create", net_sock_udp_create_native)
        || !register_function(library_object, "sock_udp_close", net_sock_udp_close_native)
        || !register_function(library_object, "sock_udp_recv", net_sock_udp_recv_native)
        || !register_function(library_object, "sock_udp_send", net_sock_udp_send_native)
        || !register_function(library_object, "coap_parse", net_coap_parse_native)
        || !register_function(library_object, "coap_get_code_raw", net_coap_get_code_raw_native)
        || !register_function(library_object, "coap_reply_simple", net_coap_reply_simple_native)) {
        jerry_release_value(constructor);
        jerry_release_value(library_object);
        return 0;
    }

    // put library as property into global object
    jerry_value_t global_object = jerry_get_global_object();
    jerry_value_t property_name_library = jerry_create_string((const jerry_char_t*) ("net"));
    jerry_value_t res = jerry_set_property(global_object, property_name_library, library_object);
    int ok = !jerry_value_is_error(res);

    // cleanup
    jerry_release_value(constructor);
    jerry_release_value(library_object);
    jerry_release_value(global_object);
    jerry_release_value(property_name_library);
    jerry_release_value(res);

    return ok;
}

// the client is part of the host application, it requests the dew point from the server in the JS application
static char client_stack[THREAD_STACKSIZE_DEFAULT];
static mutex_t client_done = MUTEX_INIT_LOCKED;

static void* client(void* arg) {
    (void) arg;

    sock_udp_ep_t remote = { .family = AF_INET6, .port = COAP_PORT };
    memcpy(remote.addr.ipv6, &ipv6_addr_loopback, sizeof(remote.addr.ipv6));

    nanocoap_sock_t sock;
    if (nanocoap_sock_connect(&sock, NULL, &remote) < 0) {
        printf("Could not connect to server!\n");
    } else {
        char payload[16];
        ssize_t len = nanocoap_sock_get(&sock, "/dew_point", payload, sizeof(payload) - 1);
        nanocoap_sock_close(&sock);
        if (len < 0) {
            printf("Request failed (%d)!\n", (int) len);
        } else {
            payload[len] = '\0';
            printf("Dew point: %s °C\n", payload);
        }
    }

    mutex_unlock(&client_done);
    return NULL;
}

int main(void) {
#if (defined(MEASURE_CPU_CYCLES) && MEASURE_CPU_CYCLES > 0) || (defined(MEASURE_HEAP) && MEASURE_HEAP > 0)
    measurements_start();
#endif

    jerry_value_t main_app, ret_value;
    int res = 0;

    // initialize engine
    jerry_init(JERRY_INIT_EMPTY);

    // register print function
    jerryx_handler_register_global((const jerry_char_t *) "print",
                                   jerryx_handler_print);

    // register natives
    if (!register_natives()) {
        printf("Could not register natives!\n");
        res = 1;
    }

    // start client, with a lower priority so that it runs as soon as the server waits for requests
    thread_create(client_stack, sizeof(client_stack), THREAD_PRIORITY_MAIN + 1, 0, client, NULL, "client");

    // parse and run main.js (the server)
    main_app = jerry_parse(NULL, 0, main_js, main_js_len, JERRY_PARSE_NO_OPTS);
    if (!jerry_value_is_error(main_app)) {
        ret_value = jerry_run(main_app);

        if (jerry_value_is_error(ret_value)) {
            printf("Could not run main script!\n");
            res = 1;
        }

        jerry_release_value(ret_value);
    }
    jerry_release_value(main_app);

    // wait for the client to print the response
    mutex_lock(&client_done);

    // cleanup engine
    jerry_cleanup();

#if (defined(MEASURE_CPU_CYCLES) && MEASURE_CPU_CYCLES > 0) || (defined(MEASURE_HEAP) && MEASURE_HEAP > 0)
    measurements_stop();
#endif
#if defined(RESET_LOOP) && RESET_LOOP > 0
    pm_reboot();
#endif

    return res;
}
