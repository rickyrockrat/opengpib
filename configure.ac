# this is example-file: configure.ac

# initial information about the project
AC_INIT([open-gpib], [0.12], [gpib@rickyrockrat.net])
AM_INIT_AUTOMAKE([AC_PACKAGE_NAME()], [AC_PACKAGE_TARNAME()], [AC_PACKAGE_VERSION()])

# check if the source folder is available
AC_CONFIG_SRCDIR([lib/open-gpib.c])

AC_CONFIG_MACRO_DIR([m4])

AM_CONFIG_HEADER([config.h])
AC_PROG_INTLTOOL([0.23])

Check for tools:
AC_PROG_CC
#AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
PKG_PROG_PKG_CONFIG([0.20])

# allow per-target cflags
AM_PROG_CC_C_O

# make the build quiet
#AM_SILENT_RULES([yes])

# use the C compiler for the following checks
AC_LANG([C])

# Checks for header files.
AC_HEADER_STDC

# Check for header files
AC_CHECK_HEADERS([arpa/inet.h asm/termbits.h ctype.h errno.h fcntl.h inttypes.h math.h netinet/in.h string.h sys/socket.h sys/stat.h sys/types.h termios.h unistd.h  ])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

  # Library versioning
LIB_CUR="0"
LIB_REV="0"
LIB_AGE="0"
AC_SUBST(LIB_CUR)
AC_SUBST(LIB_REV)
AC_SUBST(LIB_AGE)

# our version
#AC_DEFINE_SUBST([VERSION], [AC_PACKAGE_VERSION], [Program Version])
VERSION=AC_PACKAGE_VERSION

#Add support for --enable/--disable-shared
AM_PROG_LIBTOOL

#Check for and setup libraries
# First, our own library
LIBOPEN_GPIB_CFLAGS="-I\$(top_srcdir)/lib -D_REENTRANT"
LIBOPEN_GPIB_LIBS="\$(top_srcdir)/lib/libopen-gpib.la"
AC_SUBST([LIBOPEN_GPIB_CFLAGS])
AC_SUBST([LIBOPEN_GPIB_LIBS])

# now external libraries
pkg_modules="libla2vcd2 >= 0.0.0"
PKG_CHECK_MODULES([LIBLA2VCD2], [$pkg_modules],[have_libla2vcd2=yes],[have_libla2vcd2=no])
AM_CONDITIONAL(HAVE_LIBLA2VCD2, test x$enable_libla2vcd2 = xyes)

if test "x$have_libla2vcd2" = "xyes" ; then
  AC_DEFINE(HAVE_LIBLA2VCD2, 1, [ Have libla2vcd2])
  AC_SUBST(LIBLA2VCD2_CFLAGS)
  AC_SUBST(LIBLA2VCD2_LIBS)
else
  echo "WARNING: VCD OUTPUT is DISABLED. libla2vcd2 was NOT FOUND"
fi



# check for symbol in library...
#gtk_libs=gdk-x11-2.0
#AC_CHECK_LIB([$gtk_libs],gtk_status_icon_new,[],AC_MSG_ERROR(Requirement not met: gtk >= 2.10.0))



#AC_CONFIG_FILES([src/Makefile Makefile lib/Makefile lib/libopen-gpib.pc lib/interfaces/Makefile])
AC_CONFIG_FILES([src/Makefile Makefile lib/Makefile lib/libopen-gpib.pc ])

AC_OUTPUT




# distribute additional compiler and linker flags among Makefiles
# --> set and change these variables instead of CXXFLAGS or LDFLAGS (for user only)
#AC_SUBST([AM_CXXFLAGS])
#AC_SUBST([AM_LDFLAGS])


