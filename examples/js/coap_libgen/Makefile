# application name
APPLICATION = js_coap_libgen

# default BOARD environment
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../external/RIOT
EXTERNAL_MODULE_DIRS ?= $(CURDIR)/external_modules $(CURDIR)/../../../external

USEPKG += jerryscript

USEMODULE += js_libs

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += netdev_default

# GNRC
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules
USEMODULE += gnrc_ipv6_default
# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo

# increase stack size, interpreter, CoAP, network stack gets pretty big
ifeq ($(BOARD),native)
	CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=4096
else
	CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=2048
endif

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 0

# measurements for thesis evaluation
ifeq ($(BOARD),nucleo-f429zi)
	MEASURE_SYSTICKS ?= 1
	MEASURE_HEAP ?= 0
	RESET_LOOP ?= 1

	CFLAGS += -DRESET_LOOP=$(RESET_LOOP)
	ifeq ($(MEASURE_SYSTICKS),1)
		CFLAGS += -DMEASURE_SYSTICKS=1
	endif
	ifeq ($(MEASURE_HEAP),1)
		USEMODULE += malloc_monitor
		CFLAGS += -DMEASURE_HEAP=1
	endif
endif

# generate .js.h header files of .js files
BLOBS += $(wildcard *.js)

include $(RIOTBASE)/Makefile.include
