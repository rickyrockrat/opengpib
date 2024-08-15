# the subdirectories of the project to go into
SUBDIRS =  lib src
all: 
	for s in $(SUBDIRS); do make -C $$s; done

clean: 
	for s in $(SUBDIRS); do make -C $$s $@; done

install:
	for s in $(SUBDIRS); do make -C $$s $@ DESTDIR="$"DESTDIR)"; done

