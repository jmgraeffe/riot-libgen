#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "wasm_export.h"
#include "blob/hello.wasm.h"
#include "mutex.h"
#include "thread.h"
#include "net/ipv6/addr.h"
#include "net/nanocoap.h"
#include "net/nanocoap_sock.h"
#include "net/sock/udp.h"

#if (defined(MEASURE_CPU_CYCLES) && MEASURE_CPU_CYCLES > 0) || (defined(MEASURE_HEAP) && MEASURE_HEAP > 0)
#include "../../../external/measurements.h"
#endif
#if defined(RESET_LOOP) && RESET_LOOP > 0
#include "periph/pm.h"
#endif

bool iwasm_runtime_init(void);
void iwasm_runtime_destroy(void);
int iwasm_instance_exec_main(wasm_module_inst_t, int, char**);

static wasm_module_inst_t wasm_module_inst = NULL;

#define BUFFER_SIZE (128)
#define MAX_INSTANCES (2)

// instances live in these tables, and are passed to WASM as their index + 1, so that 0 means NULL
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

static int32_t free_instance(bool* used, int64_t handle) {
    if (!is_instance(used, handle)) {
        return 0;
    }
    used[handle - 1] = false;
    return 1;
}

// same layout as in the WASM application
typedef struct {
    int32_t family;
    uint16_t netif;
    uint16_t port;
} guest_ep_t;

// init is passed as offset in the WASM memory, as WAMR would not convert 0 to NULL for pointers (*)
int64_t net_new_sock_udp_ep_native(wasm_exec_env_t exec_env, uint32_t init) {
    const guest_ep_t* guest_init = NULL;
    if (init != 0) {
        wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
        if (!wasm_runtime_validate_app_addr(module_inst, init, sizeof(guest_ep_t))) {
            return 0;
        }
        guest_init = wasm_runtime_addr_app_to_native(module_inst, init);
    }
    int64_t handle = add_instance(eps_used);
    if (handle > 0) {
        sock_udp_ep_t* ep = &eps[handle - 1];
        memset(ep, 0, sizeof(*ep));
        if (guest_init != NULL) {
            ep->family = guest_init->family;
            ep->netif = guest_init->netif;
            ep->port = guest_init->port;
        }
    }
    return handle;
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int64_t (*func_ptr)(wasm_exec_env_t, uint32_t);
} net_new_sock_udp_ep_ptr = {
    .func_ptr = net_new_sock_udp_ep_native
};

int32_t net_free_sock_udp_ep_native(wasm_exec_env_t exec_env, int64_t ep) {
    (void) exec_env;
    return free_instance(eps_used, ep);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int32_t (*func_ptr)(wasm_exec_env_t, int64_t);
} net_free_sock_udp_ep_ptr = {
    .func_ptr = net_free_sock_udp_ep_native
};

// no initial values supported
int64_t net_new_sock_udp_native(wasm_exec_env_t exec_env, uint32_t init) {
    (void) exec_env;
    (void) init;
    return add_instance(socks_used);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int64_t (*func_ptr)(wasm_exec_env_t, uint32_t);
} net_new_sock_udp_ptr = {
    .func_ptr = net_new_sock_udp_native
};

int32_t net_free_sock_udp_native(wasm_exec_env_t exec_env, int64_t sock) {
    (void) exec_env;
    return free_instance(socks_used, sock);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int32_t (*func_ptr)(wasm_exec_env_t, int64_t);
} net_free_sock_udp_ptr = {
    .func_ptr = net_free_sock_udp_native
};

// no initial values supported
int64_t net_new_buffer_native(wasm_exec_env_t exec_env, uint32_t init) {
    (void) exec_env;
    (void) init;
    int64_t handle = add_instance(buffers_used);
    if (handle > 0) {
        memset(buffers[handle - 1], 0, BUFFER_SIZE);
    }
    return handle;
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int64_t (*func_ptr)(wasm_exec_env_t, uint32_t);
} net_new_buffer_ptr = {
    .func_ptr = net_new_buffer_native
};

int32_t net_free_buffer_native(wasm_exec_env_t exec_env, int64_t buf) {
    (void) exec_env;
    return free_instance(buffers_used, buf);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int32_t (*func_ptr)(wasm_exec_env_t, int64_t);
} net_free_buffer_ptr = {
    .func_ptr = net_free_buffer_native
};

// no initial values supported
int64_t net_new_coap_pkt_native(wasm_exec_env_t exec_env, uint32_t init) {
    (void) exec_env;
    (void) init;
    return add_instance(pkts_used);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int64_t (*func_ptr)(wasm_exec_env_t, uint32_t);
} net_new_coap_pkt_ptr = {
    .func_ptr = net_new_coap_pkt_native
};

int32_t net_free_coap_pkt_native(wasm_exec_env_t exec_env, int64_t pkt) {
    (void) exec_env;
    return free_instance(pkts_used, pkt);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int32_t (*func_ptr)(wasm_exec_env_t, int64_t);
} net_free_coap_pkt_ptr = {
    .func_ptr = net_free_coap_pkt_native
};

int32_t net_sock_udp_create_native(wasm_exec_env_t exec_env, int64_t sock, int64_t local, int64_t remote, uint16_t flags) {
    (void) exec_env;
    if (!is_instance(socks_used, sock) || (local && !is_instance(eps_used, local)) || (remote && !is_instance(eps_used, remote))) {
        return -EINVAL;
    }
    return sock_udp_create(&socks[sock - 1], local ? &eps[local - 1] : NULL, remote ? &eps[remote - 1] : NULL, flags);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int32_t (*func_ptr)(wasm_exec_env_t, int64_t, int64_t, int64_t, uint16_t);
} net_sock_udp_create_ptr = {
    .func_ptr = net_sock_udp_create_native
};

void net_sock_udp_close_native(wasm_exec_env_t exec_env, int64_t sock) {
    (void) exec_env;
    if (is_instance(socks_used, sock)) {
        sock_udp_close(&socks[sock - 1]);
    }
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    void (*func_ptr)(wasm_exec_env_t, int64_t);
} net_sock_udp_close_ptr = {
    .func_ptr = net_sock_udp_close_native
};

int64_t net_sock_udp_recv_native(wasm_exec_env_t exec_env, int64_t sock, int64_t data, uint32_t max_len, uint32_t timeout, int64_t remote) {
    (void) exec_env;
    if (!is_instance(socks_used, sock) || !is_instance(buffers_used, data) || max_len > BUFFER_SIZE || (remote && !is_instance(eps_used, remote))) {
        return -EINVAL;
    }
    return sock_udp_recv(&socks[sock - 1], buffers[data - 1], max_len, timeout, remote ? &eps[remote - 1] : NULL);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int64_t (*func_ptr)(wasm_exec_env_t, int64_t, int64_t, uint32_t, uint32_t, int64_t);
} net_sock_udp_recv_ptr = {
    .func_ptr = net_sock_udp_recv_native
};

int64_t net_sock_udp_send_native(wasm_exec_env_t exec_env, int64_t sock, int64_t data, uint32_t len, int64_t remote) {
    (void) exec_env;
    if (!is_instance(socks_used, sock) || !is_instance(buffers_used, data) || len > BUFFER_SIZE || (remote && !is_instance(eps_used, remote))) {
        return -EINVAL;
    }
    return sock_udp_send(&socks[sock - 1], buffers[data - 1], len, remote ? &eps[remote - 1] : NULL);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int64_t (*func_ptr)(wasm_exec_env_t, int64_t, int64_t, uint32_t, int64_t);
} net_sock_udp_send_ptr = {
    .func_ptr = net_sock_udp_send_native
};

int64_t net_coap_parse_native(wasm_exec_env_t exec_env, int64_t pkt, int64_t buf, uint32_t len) {
    (void) exec_env;
    if (!is_instance(pkts_used, pkt) || !is_instance(buffers_used, buf) || len > BUFFER_SIZE) {
        return -EINVAL;
    }
    return coap_parse(&pkts[pkt - 1], buffers[buf - 1], len);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int64_t (*func_ptr)(wasm_exec_env_t, int64_t, int64_t, uint32_t);
} net_coap_parse_ptr = {
    .func_ptr = net_coap_parse_native
};

uint32_t net_coap_get_code_raw_native(wasm_exec_env_t exec_env, int64_t pkt) {
    (void) exec_env;
    if (!is_instance(pkts_used, pkt)) {
        return 0;
    }
    return coap_get_code_raw(&pkts[pkt - 1]);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    uint32_t (*func_ptr)(wasm_exec_env_t, int64_t);
} net_coap_get_code_raw_ptr = {
    .func_ptr = net_coap_get_code_raw_native
};

int64_t net_coap_reply_simple_native(wasm_exec_env_t exec_env, int64_t pkt, uint32_t code, int64_t buf, uint32_t len, uint32_t ct, const void* payload, uint32_t payload_len) {
    (void) exec_env;
    // WAMR checks the whole payload (*~)
    if (!is_instance(pkts_used, pkt) || !is_instance(buffers_used, buf) || len > BUFFER_SIZE) {
        return -EINVAL;
    }
    return coap_reply_simple(&pkts[pkt - 1], code, buffers[buf - 1], len, ct, payload, payload_len);
}
// union just to avoid ISO C warning when initializing void* with function pointer
// (on some systems, function pointers may be different from data pointers so this might not be universal)
static union {
    void* void_ptr;
    int64_t (*func_ptr)(wasm_exec_env_t, int64_t, uint32_t, int64_t, uint32_t, uint32_t, const void*, uint32_t);
} net_coap_reply_simple_ptr = {
    .func_ptr = net_coap_reply_simple_native
};

// the client is part of the host application, it requests the dew point from the server in the WASM application
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

    NativeSymbol native_symbols[] = {
        {
            "net_new_sock_udp_ep",
            net_new_sock_udp_ep_ptr.void_ptr,
            "(i)I", // int64_t net_new_sock_udp_ep(init)
            NULL
        },
        {
            "net_free_sock_udp_ep",
            net_free_sock_udp_ep_ptr.void_ptr,
            "(I)i", // int net_free_sock_udp_ep(ep)
            NULL
        },
        {
            "net_new_sock_udp",
            net_new_sock_udp_ptr.void_ptr,
            "(i)I", // int64_t net_new_sock_udp(init)
            NULL
        },
        {
            "net_free_sock_udp",
            net_free_sock_udp_ptr.void_ptr,
            "(I)i", // int net_free_sock_udp(sock)
            NULL
        },
        {
            "net_new_buffer",
            net_new_buffer_ptr.void_ptr,
            "(i)I", // int64_t net_new_buffer(init)
            NULL
        },
        {
            "net_free_buffer",
            net_free_buffer_ptr.void_ptr,
            "(I)i", // int net_free_buffer(buf)
            NULL
        },
        {
            "net_new_coap_pkt",
            net_new_coap_pkt_ptr.void_ptr,
            "(i)I", // int64_t net_new_coap_pkt(init)
            NULL
        },
        {
            "net_free_coap_pkt",
            net_free_coap_pkt_ptr.void_ptr,
            "(I)i", // int net_free_coap_pkt(pkt)
            NULL
        },
        {
            "net_sock_udp_create",
            net_sock_udp_create_ptr.void_ptr,
            "(IIIi)i", // int net_sock_udp_create(sock, local, remote, flags)
            NULL
        },
        {
            "net_sock_udp_close",
            net_sock_udp_close_ptr.void_ptr,
            "(I)", // void net_sock_udp_close(sock)
            NULL
        },
        {
            "net_sock_udp_recv",
            net_sock_udp_recv_ptr.void_ptr,
            "(IIiiI)I", // ssize_t net_sock_udp_recv(sock, data, max_len, timeout, remote)
            NULL
        },
        {
            "net_sock_udp_send",
            net_sock_udp_send_ptr.void_ptr,
            "(IIiI)I", // ssize_t net_sock_udp_send(sock, data, len, remote)
            NULL
        },
        {
            "net_coap_parse",
            net_coap_parse_ptr.void_ptr,
            "(IIi)I", // ssize_t net_coap_parse(pkt, buf, len)
            NULL
        },
        {
            "net_coap_get_code_raw",
            net_coap_get_code_raw_ptr.void_ptr,
            "(I)i", // unsigned net_coap_get_code_raw(pkt)
            NULL
        },
        {
            "net_coap_reply_simple",
            net_coap_reply_simple_ptr.void_ptr,
            "(IiIii*~)I", // ssize_t net_coap_reply_simple(pkt, code, buf, len, ct, payload, payload_len)
            NULL
        }
    };

    uint8_t* bytecode = malloc(hello_wasm_len);
    size_t bytecode_len = hello_wasm_len;
    memcpy(bytecode, hello_wasm, bytecode_len);

    if (!iwasm_runtime_init()) {
        printf("Could not initialize WASM runtime!\n");
        return 1;
    }

    // register natives
    if (!wasm_runtime_register_natives("env", native_symbols, sizeof(native_symbols) / sizeof(NativeSymbol))) {
        printf("Could not register natives!\n");
        return 0;
    }

    // load WASM module (the compiled WASM binary)
    wasm_module_t wasm_module = NULL;
    {
        char error_buf[128];
        if (!(wasm_module = wasm_runtime_load(bytecode, bytecode_len,
                                              error_buf, sizeof(error_buf)))) {
            puts(error_buf);
            return 1;
        }
    }

    // instantiate WASM module
    {
        char error_buf[128];
        if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module, 8 * 1024,
            8 * 1024, error_buf, sizeof(error_buf)))) {
            puts(error_buf);
            return 1;
        }
    }

    // start client, with a lower priority so that it runs as soon as the server waits for requests
    thread_create(client_stack, sizeof(client_stack), THREAD_PRIORITY_MAIN + 1, 0, client, NULL, "client");

    // execute main function of the loaded WASM module (the server)
    const char *exception = 0;
    //int ret = 0;
    int argc = 0;
    char** argv = NULL;
    wasm_application_execute_main(wasm_module_inst, argc, argv);
    if ((exception = wasm_runtime_get_exception(wasm_module_inst))) {
        puts(exception);
        return 1;
    }
    /*if (argc > 0) {
        ret = *((int*)argv);
    }
    printf("ret = %d\n", ret);*/

    // wait for the client to print the response
    mutex_lock(&client_done);

    // destroy the WASM module instance
    wasm_runtime_deinstantiate(wasm_module_inst);

    // unload WASM module
    wasm_runtime_unload(wasm_module);

    // destroy WASM runtime
    iwasm_runtime_destroy();

#if (defined(MEASURE_CPU_CYCLES) && MEASURE_CPU_CYCLES > 0) || (defined(MEASURE_HEAP) && MEASURE_HEAP > 0)
    measurements_stop();
#endif
#if defined(RESET_LOOP) && RESET_LOOP > 0
    pm_reboot();
#endif

    return 0;
}