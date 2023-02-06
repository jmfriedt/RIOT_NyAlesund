APPLICATION=lora_GPS

BOARD ?= lora-e5

#include ../Makefile.tests_common
include $(RIOTBASE)/tests/Makefile.tests_common

FEATURES_REQUIRED += periph_timer_periodic
FEATURES_REQUIRED += periph_wdt


#TIMER_SPEED ?= 1000000

#CFLAGS += -DTIMER_SPEED=$(TIMER_SPEED)

USEMODULE += od
USEMODULE += shell
#USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += periph_gpio  # https://www.hackster.io/ichatz/control-external-led-using-riot-os-b626da
USEMODULE += periph_gpio_irq
USEMODULE += xtimer
USEMODULE += event_thread

DRIVER ?= sx126x_stm32wl

# use SX1276 by default
USEMODULE += $(DRIVER)

include $(RIOTBASE)/Makefile.include
