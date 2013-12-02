# Binary build - This file is part of OpenGPIB.
#PACKAGE_VERSION = @PACKAGE_VERSION@

#AM_CFLAGS -Wall
#AM_CFLAGS = -I$(top_srcdir) -DPARCELLITELOCALEDIR=\""$(parcellitelocaledir)"\"

INCLUDES = $(LIBOPEN_GPIB_CFLAGS) $(LIBLA2VCD2_CFLAGS)
LDADD = $(LIBOPEN_GPIB_LIBS) $(LIBLA2VCD2_LIBS) -lm

bin_PROGRAMS =parse_16500_config networkanalyzer calc_area_under_curve get_hp_16555_waveform get_hp_1653xx_scope get_tek_waveform tek2gplot  hp16500_test

tek2gplot_SOURCES=tek2gplot.c tek2gplot.h

get_tek_waveform_SOURCES=get_tek_waveform.c get_tek_waveform.h

get_hp_1653xx_scope_SOURCES=get_hp_1653xx_scope.c get_hp_1653xx_scope.h  hp16500.c hp1653x.c

get_hp_16555_waveform_SOURCES=get_hp_16555_waveform.c get_hp_16555_waveform.h  hp16500.c hp1655x.c

calc_area_under_curve_SOURCES=calc_area_under_curve.c calc_area_under_curve.h

networkanalyzer_SOURCES=networkanalyzer.c networkanalyzer.h

parse_16500_config_SOURCES=parse_16500_config.c parse_16500_config.h hp1655x.c

hp16500_test_SOURCES=  hp16500_test.c hp16500.c hp1653x.c
 
get_tek_waveform_CFLAGS =    $(AM_CFLAGS) -D_GLOBAL_TEK_2440_







