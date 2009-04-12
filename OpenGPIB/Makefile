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

all: tek2gplot get_tek_waveform get_hp_16530_waveform calc_area_under_curve networkanalyzer

common.o: CFLAGS+=-std=c99 -D_COMMON_

common.o: common.c common.h Makefile

tek2gplot.o: get_tek_waveform.o tek2gplot.c gpib.lib

tek2gplot: common.o tek2gplot.o 
	gcc  -lm -o $@ $^
	
calc_area_under_curve: calc_area_under_curve.o common.o
	gcc -lm  -o $@ $^

serial.o: serial.h serial.c	Makefile

gpib.o: gpib.c gpib.h	Makefile

gpib.lib: gpib.o prologixs.o serial.o common.o
	gcc -Wall -nostdlib -o $@  $^  -Wl,-r;

get_tek_waveform:	get_tek_waveform.o gpib.lib
	gcc -lm -o $@ $^
	
get_hp_16530_waveform: get_hp_16530_waveform.o gpib.lib
	gcc -lm -o $@ $^

networkanalyzer: networkanalyzer.o gpib.lib
	gcc -lm -o $@ $^

.PHONY: clean
clean:
	-rm *.o tek2gplot

