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
# For details, see http://opengpib.sourceforge.net/projects
#
# Copyright (C) 2008-2009 Doug Springer <gpib@rickyrockrat.net>
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

VERSION = 0.10

PACKAGE = opengpib

TARGETS=tek2gplot get_tek_waveform get_hp_1653xx_scope get_hp_16555_waveform \
calc_area_under_curve networkanalyzer parse_16500_config hp16500

CSRCS = common.c get_hp_1653xx_scope.c get_tek_waveform.c gpib.c networkanalyzer.c \
prologixs.c serial.c tek2gplot.c calc_area_under_curve.c parse_16500_config.c \
get_hp_16555_waveform.c fileio.c
HDRS = common.h gpib.h prologixs.h serial.h hp16500.h fileio.h

MISC = COPYING README Makefile makepng runcal ChangeLog

DISTFILES = $(MISC) $(HDRS) $(CSRCS)
DISTNAME = $(PACKAGE)-$(VERSION)

CFLAGS+=-Wall -DVERSION=$(VERSION)

ifndef LA2VCD_LIB
LA2VCD_LIB=
else
LA2VCD_H=$(dir $(LA2VCD_LIB))
CFLAGS +=-I$(LA2VCD_H) -DLA2VCD_LIB=$(LA2VCD_LIB)
endif

all: help

help:
	@echo "Usage: make defs target"
	@echo "Targets are: dist clean build"
	@echo "To get the vcd output (input for gtkwave), build with"	
	@echo " make LA2VCD_LIB=/path/to/la2vcd.lib build"
	@echo " la2vcd should be available from the same location"
	@echo " you obtained this software."

build: test $(TARGETS)

common.o hp16500.o hp16500ip.o : CFLAGS+=-std=c99 -D_COMMON_

common.o: common.c common.h Makefile

tek2gplot.o: get_tek_waveform.o tek2gplot.c gpib.lib

tek2gplot: common.o tek2gplot.o 
	gcc  -lm -o $@ $^
	
calc_area_under_curve: calc_area_under_curve.o common.o
	gcc -lm  -o $@ $^

serial.o: serial.h serial.c	Makefile

gpib.o: gpib.c gpib.h	Makefile

gpib.lib: gpib.o prologixs.o serial.o common.o hp16500ip.o fileio.o
	gcc -Wall -nostdlib -o $@  $^  -Wl,-r;
	
hp16500.o: hp16500.c  hp16500.h	Makefile

hp16500ip.o: hp16500ip.c  hp16500ip.h	Makefile

hp16500.lib: hp16500.o
	gcc -Wall -nostdlib -o $@  $^  -Wl,-r;

get_tek_waveform:	get_tek_waveform.o gpib.lib
	gcc -lm -o $@ $^
	
get_hp_1653xx_scope.o get_hp_16555_waveform.o parse_16500_config.o: hp16500.h


get_hp_1653xx_scope: get_hp_1653xx_scope.o gpib.lib hp16500.lib $(LA2VCD_LIB)
	gcc -lm -o $@ $^
	
get_hp_16555_waveform: get_hp_16555_waveform.o gpib.lib  hp16500.lib $(LA2VCD_LIB)
	gcc -lm -o $@ $^
	
hp16500.main.o: hp16500.c  hp16500.h	Makefile
	gcc $(CFLAGS) -DMAIN_QUERY  -c -o $@ hp16500.c

hp16500: hp16500.main.o gpib.lib
	gcc -lm -o $@ $^
	
test: test.o gpib.lib
	gcc -lm -o $@ $^

%.o: Makefile

#parse_16500_config.o: parse_16500_config.c 
#	gcc -Wall -c  -o $@ $<

parse_16500_config:parse_16500_config.o hp16500.lib gpib.lib $(LA2VCD_LIB)
	gcc -lm -o $@ $^

networkanalyzer: networkanalyzer.o gpib.lib
	gcc -lm -o $@ $^

.PHONY: clean
clean:
	-rm *.o *.lib $(TARGETS)

dist: $(DISTFILES)
	-rm -rf $(DISTNAME)
	mkdir $(DISTNAME)
	for f in $(DISTFILES); do ln $$f $(DISTNAME)/$$f; done
	tar --gzip -chf $(DISTNAME).tar.gz $(DISTNAME)
	-rm -rf $(DISTNAME)

