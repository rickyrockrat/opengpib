# the subdirectories of the project to go into
prefix?=/usr
SUBDIRS =  lib src
all: 
	for s in $(SUBDIRS); do make -C $$s; done

clean: 
	for s in $(SUBDIRS); do make -C $$s $@; done

install:
	for s in $(SUBDIRS); do make -C $$s $@ prefix=$(prefix) DESTDIR="$"DESTDIR)"; done

