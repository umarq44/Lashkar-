# Needs devkitARM (devkitPro). Run "make" in a devkitPro / MSYS2 shell.
PREFIX  ?= arm-none-eabi-
CC       = $(PREFIX)gcc
OBJCOPY  = $(PREFIX)objcopy
GBAFIX  ?= gbafix

all: lashkar.gba

lashkar.elf: main.c
	$(CC) -mthumb -mthumb-interwork -O2 -Wall -specs=gba.specs main.c -o $@

lashkar.gba: lashkar.elf
	$(OBJCOPY) -O binary $< $@
	-$(GBAFIX) $@ -tLASHKAROFASH

clean:
	rm -f lashkar.elf lashkar.gba
