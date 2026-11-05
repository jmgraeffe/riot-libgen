APPLICATION = wasm_coap_libgen

# If no BOARD is defined in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../external/RIOT
EXTERNAL_MODULE_DIRS ?= $(CURDIR)/external_modules $(CURDIR)/../../../external

USEPKG += wamr

USEMODULE += wasm_libs

# GNRC (server and client communicate over the loopback address, so no network device is needed)
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_sock_udp

# CoAP client in the host application
USEMODULE += nanocoap_sock

# increase stack size, VM, CoAP, network stack gets pretty big
CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=8192

export WAMR_CONFIG := $(abspath config.cmake)

BLOBS += hello.wasm

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 0

# measurements for thesis evaluation
ifneq ($(BOARD),native)
	MEASURE_CPU_CYCLES ?= 0
	MEASURE_HEAP ?= 0
	RESET_LOOP ?= 0

	CFLAGS += -DRESET_LOOP=$(RESET_LOOP)
	ifeq ($(MEASURE_CPU_CYCLES),1)
		CFLAGS += -DMEASURE_CPU_CYCLES=1
	endif
	ifeq ($(MEASURE_HEAP),1)
		USEMODULE += malloc_monitor
		CFLAGS += -DMEASURE_HEAP=1
	endif
endif

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

all: hello.wasm

hello.wasm: wasm_sample/hello.c wasm_sample/Makefile
	make -C wasm_sample hello.wasm
	mv wasm_sample/hello.wasm .

include $(RIOTBASE)/Makefile.include
