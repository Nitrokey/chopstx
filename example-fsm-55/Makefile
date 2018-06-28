# Makefile for Hacker Emblem application of Chopstx

PROJECT = hacker-emblem

CHOPSTX = ..
LDSCRIPT= hacker-emblem.ld

CSRC = reset.c hh.c

CHIP=stm32f0

# Hacker Emblem and "Happy Hacking!" demonstration
# CSRC = reset.c hh.c

# Debian logo demonstration
# CSRC = reset.c debian-logo.c

# "Hiroshi & Ayumi with Tulip" demonstration
# CSRC = reset.c hiroshi-ayumi.c

###################################
CROSS = arm-none-eabi-
CC   = $(CROSS)gcc
LD   = $(CROSS)gcc
OBJCOPY   = $(CROSS)objcopy

MCU   = cortex-m0 # -save-temps
CWARN = -Wall -Wextra -Wstrict-prototypes
DEFS  = -DMAKE_ENTRY_PUBLIC \
        -DFREE_STANDING -DMHZ=48
OPT   = -O3 -Os -g
LIBS  =

####################
include ../rules.mk

board.h:
	@echo Please make a symbolic link \'board.h\' to a file in ../board;
	@exit 1

reset.c: board.h

distclean: clean
	rm -f board.h
