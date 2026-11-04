APPLICATION = libgen_tests_wasm

BOARD ?= native

RIOTBASE ?= $(CURDIR)/../../../external/RIOT
# generated wasm_libs module (see gen.sh) and the host library exposed for testing
EXTERNAL_MODULE_DIRS += $(CURDIR)/external_modules $(CURDIR)/../modules

USEPKG += wamr
USEMODULE += wasm_libs

CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=8192

DEVELHELP ?= 1

QUIET ?= 1

BLOBS += guest.wasm

# the guest application, compiled from guest/main.c
CLANG ?= clang
GUEST_FLAGS = --target=wasm32-unknown-unknown -Os -nostdlib -ffreestanding -fvisibility=hidden \
              -Wall -Wextra -Werror -I$(CURDIR)/external_modules/wasm_libs/include \
              -Wl,--no-entry -Wl,--export=main -Wl,--export=__heap_base -Wl,--export=__data_end \
              -Wl,--allow-undefined -Wl,--export-dynamic -Wl,--strip-all \
              -Wl,-z,stack-size=4096 -Wl,--initial-memory=65536

all: guest.wasm

guest.wasm: guest/main.c $(CURDIR)/external_modules/wasm_libs/include/wasm_libs_runtime.h
	$(CLANG) $(GUEST_FLAGS) -o $@ guest/main.c

include $(RIOTBASE)/Makefile.include
