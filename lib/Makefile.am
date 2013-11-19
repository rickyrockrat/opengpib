# Library build 

#put headers in $(prefix)/include/open-gpib

hdrdir = $(includedir)/open-gpib
hdr_HEADERS = open-gpib.h


lib_LTLIBRARIES = libopen-gpib.la
libopen_gpib_la_SOURCES =  open-gpib.c prologixs.c open-gpib-ip.c serial.c hp16500ip.c fileio.c
libopen_gpib_la_LDFLAGS=-version-info 0:0:0


# pkg-config
pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=libopen-gpib.pc





