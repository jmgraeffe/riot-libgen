# application name
APPLICATION = js_coap_manual

# default BOARD environment
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../external/RIOT
EXTERNAL_MODULE_DIRS ?= $(CURDIR)/../../../external

USEPKG += jerryscript

USEMODULE += nanocoap
USEMODULE += sock_udp

# GNRC (server and client communicate over the loopback address, so no network device is needed)
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_sock_udp

# CoAP client in the host application
USEMODULE += nanocoap_sock

# increase stack size, interpreter, CoAP, network stack gets pretty big
ifneq (,$(filter native native64,$(BOARD)))
	CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=4096
else
	CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=2048
endif

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

# generate .js.h header files of .js files
BLOBS += $(wildcard *.js)

include $(RIOTBASE)/Makefile.include
