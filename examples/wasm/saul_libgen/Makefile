APPLICATION = wasm_saul_libgen

# If no BOARD is defined in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../external/RIOT
EXTERNAL_MODULE_DIRS ?= $(CURDIR)/external_modules $(CURDIR)/../../../external

USEPKG += wamr

USEMODULE += wasm_libs
USEMODULE += bme680_i2c
USEMODULE += printf_float

# increase stack size, VM & printf are pretty big
CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=4096

export WAMR_CONFIG := $(abspath config.cmake)

BLOBS += hello.wasm

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

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

all: hello.wasm

hello.wasm: wasm_sample/hello.c wasm_sample/Makefile
	make -C wasm_sample hello.wasm
	mv wasm_sample/hello.wasm .

include $(RIOTBASE)/Makefile.include
