# Top level make

# set the options for automake..
#Hook dist-bzip2 to dist (dist-bzip2).
#set gnu strictness (gnu)
AUTOMAKE_OPTIONS =  dist-bzip2
# the subdirectories of the project to go into
SUBDIRS =  lib src

EXTRA_DIST = autogen.sh po/Makefile.in.in po/POTFILES.in

dist_doc_DATA = README

DISTCLEANFILES = po/Makefile po/Makefile.in po/stamp-it po/POTFILES


ACLOCAL_AMFLAGS = -I m4