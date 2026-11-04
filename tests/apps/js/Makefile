APPLICATION = libgen_tests_js

BOARD ?= native

RIOTBASE ?= $(CURDIR)/../../../external/RIOT
# generated js_libs module (see gen.sh) and the host library exposed for testing
EXTERNAL_MODULE_DIRS += $(CURDIR)/external_modules $(CURDIR)/../modules

USEPKG += jerryscript
USEMODULE += js_libs

JERRY_STACK = 0

CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=4096

DEVELHELP ?= 1

QUIET ?= 1

BLOBS += main.js

include $(RIOTBASE)/Makefile.include
