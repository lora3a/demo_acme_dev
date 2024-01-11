APPLICATION = node-NOSENSOR
BOARD ?= berta-h10

BASE ?= $(CURDIR)/../../
LORA3ABASE ?= $(BASE)/../lora3a-boards
RIOTBASE ?= $(BASE)/../RIOT

EXTERNAL_BOARD_DIRS=$(LORA3ABASE)/boards
EXTERNAL_MODULE_DIRS+=$(LORA3ABASE)/modules
EXTERNAL_MODULE_DIRS+=$(BASE)/shared_modules
EXTERNAL_PKG_DIRS=$(LORA3ABASE)/pkg

QUIET ?= 1
PORT ?= /dev/ttyUSB0
CFLAGS += -DACME_DEBUG=0

USEMODULE += fmt
USEMODULE += periph_adc
USEMODULE += periph_spi_reconfigure
USEMODULE += periph_cpuid
USEMODULE += printf_float
USEMODULE += saml21_backup_mode
USEMODULE += acme_lora
USEMODULE += nodes_data
USEMODULE += h10_adc

CFLAGS += -DSLEEP_TIME=300

BW ?= 2
CH ?= 868100000L

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

ifneq ($(BS),)
	CFLAGS+=-DDEFAULT_LORA_BOOST=$(BS)
endif

include $(RIOTBASE)/Makefile.include
include $(LORA3ABASE)/Makefile.include
