# application name
APPLICATION = js_saul_manual

JERRY_STACK = 0

# default BOARD environment
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../external/RIOT
EXTERNAL_MODULE_DIRS ?= $(CURDIR)/../../../external

USEPKG += jerryscript

USEMODULE += easysaul
USEMODULE += bme680_i2c

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# generate .js.h header files of .js files
BLOBS += $(wildcard *.js)

include $(RIOTBASE)/Makefile.include
