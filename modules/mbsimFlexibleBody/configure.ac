dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.57])
AC_INIT([mbsimFlexibleBody],[4.0.0],[ts.schindler@gmail.com])
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([mbsimFlexibleBody/Makefile.am])
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

PKG_CHECK_MODULES(DEPS, mbsim >= 3.3.1)
requires="mbsim"

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

dnl check for optional module nurbs
PKG_CHECK_MODULES([NURBS], [nurbs++ >= 3.0.13], 
                  [requires="$requires nurbs++"; nurbs_compilerflag="-DHAVE_NURBS"; AC_DEFINE([HAVE_NURBS],[1],[linking with NURBS])], 
                  [nurbsPKGCONF="No"; AC_MSG_RESULT([No Nurbs-library found. Some flexible contours not possible.])])
AM_CONDITIONAL([COND_NURBS], [test _$nurbs_compilerflag = _-DHAVE_NURBS])
AC_SUBST([nurbs_compilerflag])


dnl Old - check NURBS Package
if (test "_$nurbsPKGCONF" = "_No"); then
  AC_ARG_WITH(nurbs-inc-prefix,[  --with-nurbs-inc-prefix=PFX  Prefix where nurbs includes are installed], [nurbs_inc_prefix="$withval"], [nurbs_inc_prefix="$prefix/include"])
  
  AC_ARG_WITH(nurbs-lib-prefix,[  --with-nurbs-lib-prefix=PFX  Prefix where nurbs libraries are installed], [nurbs_lib_prefix="$withval"], [nurbs_lib_prefix="$prefix/lib"])
  
  AC_MSG_CHECKING([for library nurbs without pkg-config support])
  CPPFLAGS_OLD="$CPPFLAGS"
  LDFLAGS_OLD="$LDFLAGS"
  LIBS_OLD="$LIBS"
  
  CPPFLAGS="$CPPFLAGS -I$nurbs_inc_prefix/nurbs++"
  LDFLAGS="$LDFLAGS -L$nurbs_lib_prefix"
  LIBS="$LIBS -lnurbsd -lnurbsf -lmatrixN -lmatrixI -lmatrix"
  
  AC_LINK_IFELSE([AC_LANG_PROGRAM([#include "nurbs.h"],[PlNurbsCurvef var1; PlNurbsCurved var2;])],[AC_MSG_RESULT([yes]); nurbs="yes"],[AC_MSG_RESULT([no]); nurbs="no"])
  
  if (test "$nurbs" = "yes"); then
    AC_DEFINE([HAVE_NURBS],[1],[linking with NURBS])
  else
    CPPFLAGS="$CPPFLAGS_OLD"
    LDFLAGS="$LDFLAGS_OLD"
    LIBS="$LIBS_OLD"
  fi
fi

dnl versioning (see http://www.gnu.org/software/libtool/manual.html)
dnl current: 	The most recent interface number that this library implements. 
dnl revision: 	The implementation number of the current interface. 
dnl age: 	The difference between the newest and oldest interfaces that
dnl		this library implements. In other words, the library implements all the
dnl 		interface numbers in the range from number current (age<=current).
LIBMBSIMFLEXIBLEBODY_CURRENT=0
LIBMBSIMFLEXIBLEBODY_REVISION=0
LIBMBSIMFLEXIBLEBODY_AGE=0
AC_SUBST([LIBMBSIMFLEXIBLEBODY_CURRENT])
AC_SUBST([LIBMBSIMFLEXIBLEBODY_REVISION])
AC_SUBST([LIBMBSIMFLEXIBLEBODY_AGE])
LIBMBSIMFLEXIBLEBODY_VERSION_INFO=$LIBMBSIMFLEXIBLEBODY_CURRENT:$LIBMBSIMFLEXIBLEBODY_REVISION:$LIBMBSIMFLEXIBLEBODY_AGE
AC_SUBST([LIBMBSIMFLEXIBLEBODY_VERSION_INFO])

CPPFLAGS="$CPPFLAGS"

dnl needed for XML
AC_SUBST([MBXMLUTILSBINDIR],[$(pkg-config mbxmlutils --variable BINDIR)])
AC_SUBST([MBXMLUTILSDOCDIR],[$(pkg-config mbxmlutils --variable DOCDIR)])
AC_SUBST([MBXMLUTILSSCHEMADIR],[$(pkg-config mbxmlutils --variable SCHEMADIR)])
AM_CONDITIONAL([COND_XML], [test "_$MBXMLUTILSSCHEMADIR" != "_"])

CPPFLAGS="$CPPFLAGS -pedantic -Wall -Wno-unknown-pragmas -Wno-long-long -Wfloat-equal"
AC_MSG_CHECKING([for -Wno-strict-overflow compiler flag])
CPPFLAGS_OLD=$CPPFLAGS
CPPFLAGS="$CPPFLAGS -Wno-strict-overflow"
AC_COMPILE_IFELSE([int main() {}], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no]); CPPFLAGS=$CPPFLAGS_OLD])

AC_ARG_WITH([doxygenpath],[  --with-doxygenpath=PATH  The path to the 'doxygen' program.],[doxygenpath="$withval"],[doxygenpath=""])
AC_PATH_PROG([doxygen],[doxygen],[no],[$doxygenpath:$PATH])
if test $doxygen = "no" ; then
  AC_MSG_RESULT([cannot generate documentation. 'doxygen' required.])
fi

AC_SUBST([requires])

LDFLAGS="$LDFLAGS -no-undefined -Wl,--no-undefined"
test "_$host_os" != "_mingw32" && LDFLAGS="$LDFLAGS -Wl,--disable-new-dtags,-rpath,\\\$\$ORIGIN/../lib"

AC_CONFIG_FILES([Makefile mbsimFlexibleBody/Makefile mbsimFlexibleBody/flexible_body/Makefile mbsimFlexibleBody/flexible_body/finite_elements/Makefile mbsimFlexibleBody/flexible_body/finite_elements/finite_element_1s_33_rcm/Makefile mbsimFlexibleBody/flexible_body/finite_elements/finite_element_2s_13_mfr_mindlin/Makefile mbsimFlexibleBody/contact_kinematics/Makefile mbsimFlexibleBody/contours/Makefile mbsimFlexibleBody/contours/neutral_contour/Makefile mbsimFlexibleBody/contours/neutral_contour/neutral_contour_components/Makefile mbsimFlexibleBody/utils/Makefile mbsimFlexibleBody.pc schema/Makefile xmldoc/Makefile doc/Makefile doc/doxyfile xmldoc/Doxyfile])

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
