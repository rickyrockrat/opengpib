# Library build - This file is part of OpenGPIB.

#put headers in $(prefix)/include/open-gpib
hdrdir = $(includedir)/open-gpib
hdr_HEADERS = open-gpib.h

# Since I can't figure out how to pass per-source file cflag options, I'm forced to build 
# two libraries - one for my cflags, and one for all the rest.

noinst_LTLIBRARIES = liba.la 
liba_la_SOURCES = prologixs.c open-gpib-inet.c serial.c hp16500ip.c fileio.c open-gpib-param.c support.c #config.c
liba_la_CFLAGS = $(AM_CFLAGS) 
#LDADD = -fPIC

# rest of them
#libb_la_SOURCES = prologixs.c open-gpib-inet.c serial.c hp16500ip.c fileio.c 

#libopen-gpib.la: interfaces.h 
open-gpib.c : open-gpib.h

lib_LTLIBRARIES = libopen-gpib.la
libopen_gpib_la_SOURCES =  open-gpib.c
libopen_gpib_la_LDFLAGS=-version-info 0:0:0 $(LIBCONFIG_LIBS)
libopen_gpib_la_LIBADD = liba.la
libopen_gpib_la_DEPENDENCIES = liba.la 
libopen_gpib_la_CFLAGS = $(AM_CFLAGS) -D_ALLOC_REGISTER_



#EXTRA_PROGRAMS=libopen-gpib.lib

#BUILD_SOURCES = interfaces.h

#open_gpib_CFLAGS =  $(AM_CFLAGS) -D_ALLOC_REGISTER_
#ALL_SRC=$(liba_la_SOURCES:%=%.c)
#open-gpib.lo: interfaces.h
#open_gpib.lo: CFLAGS += -D_ALLOC_REGISTER_ 

interfaces.h: Makefile $(liba_la_SOURCES)	build-interfaces
	./build-interfaces $(liba_la_SOURCES)
	
open-gpib.h: interfaces.h _open-gpib.h
	./build-interfaces "header"

bin_PROGRAMS =test

test_SOURCES=test.c
test_LDADD= $(LIBOPEN_GPIB_LIBS) 

# pkg-config
pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=libopen-gpib.pc





