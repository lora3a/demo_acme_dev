APPLICATION = listener
BOARD ?= berta-h10

BASE ?= $(CURDIR)/../..
LORA3ABASE ?= $(BASE)/lora3a-boards
RIOTBASE ?= $(BASE)/RIOT

EXTERNAL_BOARD_DIRS=$(LORA3ABASE)/boards
EXTERNAL_MODULE_DIRS+=$(LORA3ABASE)/modules
EXTERNAL_PKG_DIRS=$(LORA3ABASE)/pkg
QUIET ?= 1
DEVELHELP ?= 1

USEMODULE += periph_spi_reconfigure
USEMODULE += stdio_uart

USEMODULE += acme_lora
CFLAGS+=-DMAX_PAYLOAD_LEN=200

BW ?= 2
CH ?= 869500000L
SF ?= 7
CR ?= 1

ifneq ($(BW),)
	CFLAGS+=-DDEFAULT_LORA_BANDWIDTH=$(BW)
endif

ifneq ($(SF),)
	CFLAGS+=-DDEFAULT_LORA_SPREADING_FACTOR=$(SF)
endif

ifneq ($(CR),)
	CFLAGS+=-DDEFAULT_LORA_CODERATE=$(CR)
endif

ifneq ($(CH),)
	CFLAGS+=-DDEFAULT_LORA_CHANNEL=$(CH)
endif

ifneq ($(PW),)
	CFLAGS+=-DDEFAULT_LORA_POWER=$(PW)
endif

include $(RIOTBASE)/Makefile.include
include $(LORA3ABASE)/Makefile.include
