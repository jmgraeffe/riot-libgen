# application name
APPLICATION = js_saul_libgen

JERRY_STACK = 0

# default BOARD environment
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../external/RIOT
EXTERNAL_MODULE_DIRS ?= $(CURDIR)/external_modules $(CURDIR)/../../../external

USEPKG += jerryscript

USEMODULE += js_libs
USEMODULE += bme680_i2c

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

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
