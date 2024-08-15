# Library build - This file is part of OpenGPIB.
prefix?=usr

LA2VCDLD|=$(shell pkg-config --libs libla2vcd2)
LA2VCDCF|=$(shell pkg-config --cflags libla2vcd2)

LIBS = libopen-gpib.a libopen-gpib.so
all: $(LIBS)

.PHONY: libcheck
libcheck:
	@if [ -z "$(LA2VCDLD)" ]; then echo "la2vcd2 NOT FOUND. VCD (gtkwave) data not supported"; fi
	

SRC = prologixs.c open-gpib-inet.c serial.c hp16500ip.c fileio.c open-gpib-param.c support.c
OBJ = $(patsubst %.c,%.o,$(SRC)) 
CFLAGS=-Wall -I. -I .. -fPIC -DHAVE_CONFIG_H

#ifneq($(LA2VCDLD),)
	CFLAGS+= -DHAVE_LIBLA2VCD2
#endif

config.h:
	./make-config

open-gpib.o: CFLAGS+=-D_ALLOC_REGISTER_

open-gpib.c : open-gpib.h

$(OBJ) : open-gpib.h config.h

%.o:%.c
	gcc $(CFLAGS) -c -o $@ $<

libopen-gpib.a: $(OBJ) open-gpib.o
	ar -crs $@ $^

libopen-gpib.so: $(OBJ) open-gpib.o
	gcc -Wall -shared -o $@ $^

interfaces.h: Makefile $(SRC) build-interfaces
	./build-interfaces $(SRC)
	
open-gpib.h: interfaces.h _open-gpib.h
	./build-interfaces "header"

test : test.c $(OBJ)
	gcc $(CFLAGS) -o $@ $< open-gpib.c -L . $(OBJ) $(LA2VCDLD)

# pkg-config
pkgconfigdir=$(libdir)/pkgconfig
PKGCFG=libopen-gpib.pc

install: $(LIBS)
	mkdir -p $(DESTDIR/$(prefix)/lib/pkgconfig $(DESTDIR/$(prefix)/include/open-gpib
	cp $(LIBS) $(DESTDIR/$(prefix)/lib
	cp $(PKGCFG) $(DESTDIR/$(prefix)/lib/pkgconfig
	cp open-gpib.h $(DESTDIR/$(prefix)/include/open-gpib

clean:
	-rm *.o $(LIBS) test open-gpib.h interfaces.h





