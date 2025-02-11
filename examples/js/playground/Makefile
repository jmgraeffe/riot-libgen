# application name
APPLICATION = js_playground

JERRY_STACK = 0

# default BOARD environment
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../external/RIOT
EXTERNAL_MODULE_DIRS ?= $(CURDIR)/external_modules

USEMODULE += js_libs
USEPKG += jerryscript

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# increase stack size
ifeq ($(BOARD),native)
	CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=4096
else
	CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=2048
endif

# generate .js.h header files of .js files
BLOBS += $(wildcard *.js)

include $(RIOTBASE)/Makefile.include
