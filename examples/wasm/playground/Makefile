APPLICATION = wasm_playground
# If no BOARD is defined in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../external/RIOT
EXTERNAL_MODULE_DIRS ?= $(CURDIR)/external_modules $(CURDIR)/../../../external

USEMODULE += wasm_libs
USEPKG += wamr

# increase stack size
CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=8192

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
