# \file ******************************************************************
#\n\b File:        Makefile
#\n\b Author:      Doug Springer
#\n\b Company:     DNK Designs Inc.
#\n\b Date:        10/05/2007  7:56 am
#\n\b Description: 
#*/ /************************************************************************
#Change Log: \n
#

#-std=c99
CFLAGS+=-Wall

all: tek2gplot get_tek_waveform

common.o: CFLAGS+=-std=c99 -D_COMMON_

common.o: common.c common.h

tek2gplot.o get_tek_waveform.o common.o: common.h tek2gplot.c

tek2gplot: tek2gplot.o common.o
	gcc  -lm -o $@ $^

serial.o get_tek_waveform.o: serial.h

get_tek_waveform:	serial.o get_tek_waveform.o common.o
	gcc -lm -o $@ $^

.PHONY: clean
clean:
	-rm *.o tek2gplot

