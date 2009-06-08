#!/bin/sh
#
# Makefile arguments:
#
#	ENDIAN = {big, little}
#	CFLAGS = {GNU GCC compiler flags}
#

ENDIAN = `$(SIM_DIR)/sysprobe -s`
CFLAGS = -O2

# should be either OSC_HOST_BIG_ENDIAN or OSC_HOST_LITTLE_ENDIAN
# Apple Mac OS X:  OSC_HOST_BIG_ENDIAN
# Win32:           OSC_HOST_LITTLE_ENDIAN
# i386 GNU/Linux:  OSC_HOST_LITTLE_ENDIAN
ifeq ($(ENDIAN),big)
	OSC_ENDIANNES=OSC_HOST_BIG_ENDIAN
else
	OSC_ENDIANNES=OSC_HOST_LITTLE_ENDIAN
endif

SPRING_HOME=../..
MY_BIN=OscStatsSender.bin

build: main.cpp $(SPRING_HOME)/rts/lib/oscpack/*.c*
	$(CXX) $(CFLAGS) -o $(MY_BIN) -D$(OSC_ENDIANNES) -I$(SPRING_HOME) main.cpp $(SPRING_HOME)/rts/lib/oscpack/*.c*

