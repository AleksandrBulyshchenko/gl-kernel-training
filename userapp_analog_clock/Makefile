
CROSS_COMPILE ?= arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall

.PHONY: all clean

all: analog_clock

analog_clock: analog_clock.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm analog_clock