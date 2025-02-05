APPLICATION = wasm_coap_libgen

# If no BOARD is defined in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../external/RIOT
EXTERNAL_MODULE_DIRS ?= $(CURDIR)/external_modules $(CURDIR)/../../../external

USEPKG += wamr

USEMODULE += wasm_libs

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += netdev_default

# GNRC
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules
USEMODULE += gnrc_ipv6_default
# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo

USEMODULE += xtimer

ifneq ($(BOARD),native)
	CFLAGS += -DTHREAD_STACKSIZE_MAIN='(6 * 1024)'
endif

export WAMR_CONFIG := $(abspath config.cmake)

BLOBS += hello.wasm

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

all: hello.wasm

hello.wasm: wasm_sample/hello.c wasm_sample/Makefile
	make -C wasm_sample hello.wasm
	mv wasm_sample/hello.wasm .

include $(RIOTBASE)/Makefile.include
