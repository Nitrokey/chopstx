# Makefile for example application of Chopstx

PROJECT = sample

CHOPSTX = ..
LDSCRIPT= sample.ld.m4
CSRC = sample.c usb-cdc.c

CHIP=stm32l4

USE_SYS = yes
USE_USB = yes
ENABLE_OUTPUT_HEX=yes

###################################
CROSS = arm-none-eabi-
CC   = $(CROSS)gcc
LD   = $(CROSS)gcc
OBJCOPY   = $(CROSS)objcopy

MCU   = cortex-m4
CWARN = -Wall -Wextra -Wstrict-prototypes
DEFS  = -DUSE_SYS3 -DFREE_STANDING -DMHZ=80
OPT   = -O3 -Os -g
LIBS  =

####################
include ../rules.mk

board.h:
	@echo Please make a symbolic link \'board.h\' to a file in ../board;
	@exit 1

sys.c: board.h

distclean: clean
	rm -f board.h
