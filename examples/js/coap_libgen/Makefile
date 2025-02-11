# application name
APPLICATION = js_coap_libgen

JERRY_STACK = 0

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
DEVELHELP ?= 1

ifeq (,$(filter native native64,$(BOARD)))
# Set stack size to something (conservatively) enormous
CFLAGS += -DTHREAD_STACKSIZE_MAIN=9092
endif

# generate .js.h header files of .js files
BLOBS += $(wildcard *.js)

include $(RIOTBASE)/Makefile.include
