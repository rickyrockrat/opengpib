# \file ******************************************************************
#\n\b File:        Makefile
#\n\b Author:      Doug Springer
#\n\b Company:     DNK Designs Inc.
#\n\b Date:        10/05/2007  7:56 am
#\n\b Description: 
#*/ /************************************************************************
#Change Log: \n
#

CFLAGS+=-Wall

all: tek2gplot get_tek_waveform

tek2gplot: tek2gplot.c
	gcc  -lm -o $@ $^

serial.o get_tek_waveform.o: serial.h

get_tek_waveform:	serial.o get_tek_waveform.o
	gcc -o $@ $^

.PHONY: clean
clean:
	-rm *.o tek2gplot

