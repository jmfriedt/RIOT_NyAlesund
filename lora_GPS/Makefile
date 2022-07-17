APPLICATION=lora_GPS

BOARD ?= b-l072z-lrwan1

include ../Makefile.tests_common

FEATURES_REQUIRED = periph_timer_periodic

#TIMER_SPEED ?= 1000000

#CFLAGS += -DTIMER_SPEED=$(TIMER_SPEED)

USEMODULE += od
USEMODULE += shell
#USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += periph_gpio  # https://www.hackster.io/ichatz/control-external-led-using-riot-os-b626da
USEMODULE += periph_gpio_irq
USEMODULE += xtimer

DRIVER ?= sx1276

# use SX1276 by default
USEMODULE += $(DRIVER)

include $(RIOTBASE)/Makefile.include


