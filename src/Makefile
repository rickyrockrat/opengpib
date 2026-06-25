SHELL|=/bin/bash
BINS= parse_16500_config networkanalyzer calc_area_under_curve get_hp_1653xx_scope get_tek_waveform tek2gplot hp16500_test # get_hp_16555_waveform 

LA2VCDL:=$(shell pkg-config --libs libla2vcd2)
LA2VCDC:=$(shell pkg-config --cflags libla2vcd2)
CFLAGS=-Wall $(LA2VCDC) -DHAVE_CONFIG_H -I ../lib -I .
LDFLAGS=-L ../lib
LDLIBS=-lopen-gpib  $(LA2VCDL) -lm 
HAVE_LIBLA2VCD2:=$(shell if [ -n "$(LA2VCDL)" ]; then echo '-DHAVE_LIBLA2VCD2'; fi)

all: $(BINS)


#	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

get_tek_waveform get_tek_waveform.o: CFLAGS += -D_GLOBAL_TEK_2440_

tek2gplot_SOURCES=tek2gplot.c tek2gplot.h

#get_tek_waveform_SOURCES=get_tek_waveform.c get_tek_waveform.h

get_hp_1653xx_scope:get_hp_1653xx_scope.o hp16500.o hp1653x.o

#get_hp_16555_waveform:get_hp_16555_waveform.o hp16500.o hp1655x.o

calc_area_under_curve:calc_area_under_curve.o 

networkanalyzer_SOURCES=networkanalyzer.c networkanalyzer.h

parse_16500_config:parse_16500_config.o hp1655x.o

hp16500_test : hp16500_test.o hp16500.o hp1653x.o
 
clean:
	-rm $(BINS) *.o




