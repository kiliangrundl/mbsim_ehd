dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.57])
AC_INIT([mbsimElectronics],[4.0.0],[martin.o.foerg@gmail.com])
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([mbsimElectronics/Makefile.am])
LT_INIT([win32-dll])

: ${CFLAGS=""}
: ${CXXFLAGS=""}
: ${FCFLAGS=""}
: ${FFLAGS=""}

AC_PROG_CXX
AC_PROG_F77
AC_PROG_CXXCPP
AC_LANG([C++])
AC_F77_WRAPPERS

# By autoscan
AC_C_CONST
AC_CHECK_FUNCS([sqrt])
AC_HEADER_STDBOOL

PKG_CHECK_MODULES(DEPS, mbsim >= 3.0.0 mbsimControl)

dnl Enable debugging 
AC_ARG_ENABLE([debug],[  --enable-debug  Enable debuggig],[debug="yes"],[debug="no"])

if test "_$debug" = "_yes" ; then
  CFLAGS="$CFLAGS -ggdb"
  CXXFLAGS="$CXXFLAGS -ggdb"
  FCFLAGS="$FCFLAGS -ggdb"
  FFLAGS="$FFLAGS -ggdb"
else
  CFLAGS="$CFLAGS -g -O3"
  CXXFLAGS="$CXXFLAGS -g -O3 -DNDEBUG"
  FCFLAGS="$FCFLAGS -g -O2"
  FFLAGS="$FFLAGS -g -O2"
fi

dnl versioning (see http://www.gnu.org/software/libtool/manual.html)
dnl current: 	The most recent interface number that this library implements. 
dnl revision: 	The implementation number of the current interface. 
dnl age: 	The difference between the newest and oldest interfaces that
dnl		this library implements. In other words, the library implements all the
dnl 		interface numbers in the range from number current (age<=current).
LIBMBSIMELECTRONICS_CURRENT=0
LIBMBSIMELECTRONICS_REVISION=0
LIBMBSIMELECTRONICS_AGE=0
AC_SUBST([LIBMBSIMELECTRONICS_CURRENT])
AC_SUBST([LIBMBSIMELECTRONICS_REVISION])
AC_SUBST([LIBMBSIMELECTRONICS_AGE])
LIBMBSIMELECTRONICS_VERSION_INFO=$LIBMBSIMELECTRONICS_CURRENT:$LIBMBSIMELECTRONICS_REVISION:$LIBMBSIMELECTRONICS_AGE
AC_SUBST([LIBMBSIMELECTRONICS_VERSION_INFO])

CPPFLAGS="$CPPFLAGS -Wall -Werror"

AC_ARG_WITH([doxygenpath],[  --with-doxygenpath=PATH  The path to the 'doxygen' program.],[doxygenpath="$withval"],[doxygenpath=""])
AC_PATH_PROG([doxygen],[doxygen],[no],[$doxygenpath:$PATH])
if test $doxygen = "no" ; then
  AC_MSG_RESULT([cannot generate documentation. 'doxygen' required.])
fi

LDFLAGS="$LDFLAGS -no-undefined -Wl,--no-undefined"
test "_$host_os" != "_mingw32" && LDFLAGS="$LDFLAGS -Wl,--disable-new-dtags,-rpath,\\\$\$ORIGIN/../lib"

AC_CONFIG_FILES([Makefile mbsimElectronics/Makefile doc/Makefile mbsimElectronics.pc doc/doxyfile])

AC_OUTPUT

echo "
Configuration for $PACKAGE $VERSION :
--------------------------------

 Source code location ....: $srcdir
 Host System Type ........: $host
 Preprocessor ............: $CXX $CPPFLAGS
 Compiler ................: $CXX $CXXFLAGS
 Linker ..................: $CXX $LDFLAGS $LIBS 
 Install path ............: $prefix

 Now type 'make' to build $PACKAGE $VERSION,
 and then 'make install' for installation.
"
