# \file ******************************************************************
#\n\b File:        Makefile
#\n\b Author:      Doug Springer
#\n\b Company:     DNK Designs Inc.
#\n\b Date:        10/05/2007  7:56 am
#\n\b Description: 
#*/ /************************************************************************
#Change Log: \n
#
# This file is part of OpenGPIB.
#
#    OpenGPIB is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License version 3 
#    as published by the Free Software Foundation. Note that permission 
#    is not granted to redistribute this program under the terms of any
#    other version of the General Public License.
#
#    OpenGPIB is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with OpenGPIB.  If not, see <http://www.gnu.org/licenses/>.
#    
#		The License should be in the file called COPYING.

#-std=c99
CFLAGS+=-Wall

ifndef LA2VCD_LIB
LA2VCD_LIB=
endif
all: tek2gplot get_tek_waveform get_hp_16530_waveform get_hp_16555_waveform calc_area_under_curve networkanalyzer parse_16500_config

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
	
get_hp_16530_waveform.o get_hp_16555_waveform.o parse_16500_config.o: hp16500.h

get_hp_16530_waveform: get_hp_16530_waveform.o gpib.lib
	gcc -lm -o $@ $^
	
get_hp_16555_waveform: get_hp_16555_waveform.o gpib.lib
	gcc -lm -o $@ $^

parse_16500_config.o: parse_16500_config.c Makefile
	gcc -Wall -c -DLA2VCD_LIB=$(LA2VCD_LIB) -o $@ $<

parse_16500_config:parse_16500_config.o $(LA2VCD_LIB)
	gcc -lm -o $@ $^

networkanalyzer: networkanalyzer.o gpib.lib
	gcc -lm -o $@ $^

.PHONY: clean
clean:
	-rm *.o tek2gplot

