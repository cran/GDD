# Process this file with autoconf to produce a configure script.
AC_INIT(GDD, 0.1, Simon.Urbanek@r-project.org)
AC_CONFIG_SRCDIR([src/GDD.c])
AC_CONFIG_HEADER([src/gddconfig.h])

# find R home and set CC/CFLAGS
: ${R_HOME=`R RHOME`}
if test -z "${R_HOME}"; then
  echo "could not determine R_HOME"
  exit 1
fi
CC=`${R_HOME}/bin/R CMD config CC`;
CFLAGS=`${R_HOME}/bin/R CMD config CFLAGS`
AC_SUBST(R_HOME)

# platform check
#AC_CANONICAL_HOST
#case "${host_os}" in
#  darwin*)
#    AC_DEFINE(DARWIN, 1, [Set if the OS is Darwin/Mac OS X.])
#    ;;
#esac

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([memset mkdir rmdir])

AC_PATH_PROG([GDLIBCONF],[gdlib-config],[$PATH:/usr/local/bin:ext/bin:ext])

if test "xx$GDLIBCONF" != "xx"; then

GDCFLAGS=`$GDLIBCONF --cflags`
GDLIBS=`$GDLIBCONF --libs`
GDLDFLAGS=`$GDLIBCONF --ldflags`
GDSELFINC=`$GDLIBCONF --includedir`
GDSELFLIB=`$GDLIBCONF --libdir`

if test "xx$GDSELFINC" != "xx"; then
   GDSELFINC="-I$GDSELFINC"
fi
if test "xx$GDSELFLIB" != "xx"; then
   GDSELFLIB="-L$GDSELFLIB"
fi

CPPFLAGS="$CPPFLAGS $GDCFLAGS $GDSELFINC"
LIBS="$LIBS $GDLIBS"
LDFLAGS="$LDFLAGS $GDLDFLAGS $GDSELFLIB"

echo "libgd-flags according to gdlib-config:"
echo "   $GDCFLAGS $GDLDFLAGS $GDLIBS"
echo "if that is not correct, fix your gd installation"

AC_CHECK_HEADER(gd.h,[],AC_MSG_ERROR(Can't find gd.h!))
AC_CHECK_LIB(gd, gdImageCreateFromPng,[],AC_MSG_ERROR(Can't find libgd!))

AC_MSG_CHECKING([usability of FreeType in GD])
AC_LINK_IFELSE([
#include "gd.h"
int main(void) {
    gdImageStringFT(0, 0, 0, "bla", 12.0, 0.0, 10, 10, "foo");
    return 0;
}
],[
AC_DEFINE(HAS_FTL, 1, [is freetype library available?])
AC_MSG_RESULT(yes)
],[AC_MSG_RESULT(no)
echo ""
echo "*** freetype is not available for GD, only very basic font support ***"
echo "*** will be included. You can enhance the quality of GDD images by ***"
echo "*** installing freetype library and the corresponding GD.          ***"
echo ""
])

else
AC_CHECK_LIB(m, floor)
AC_CHECK_LIB(z, deflate)

dnl ****************** Begin PNG checking ***********************
if test "x$with_png" != "x"; then
  PNG_DIR=$with_png
  for i in / /include /include/png /png; do
     test -f $with_png/$i/png.h           && PNG_INCDIR=$with_png/$i
  done
  test -f $with_png/lib/libpng.a -o -f $with_png/lib/libpng.so -o -f $with_png/lib/libpng.dylib && PNG_LIBDIR=$with_png/lib
  test -f $with_png/libpng.a -o -f $with_png/libpng.so -o -f $with_png/libpng.dylib && PNG_LIBDIR=$with_png
else
  for i in /usr/include /usr/local/include; do
    test -f $i/png.h                       && PNG_INCDIR=$i
  done
  for i in /usr /usr/local /usr/local/png; do
    test -f $i/lib/libpng.a -o -f $i/lib/libpng.so && PNG_LIBDIR=$i/lib
  done
fi

if test "x$PNG_INCDIR" == "x"; then
   PNG_INCDIR=`pwd`/ext/include
fi
if test "x$PNG_LIBDIR" == "x"; then
   PNG_LIBDIR=`pwd`/ext/lib
fi

LDFLAGS="-L$PNG_LIBDIR $LDFLAGS"
dnl CFLAGS="-I$PNG_INCDIR $CFLAGS"
CPPFLAGS="$CPPFLAGS -I$PNG_INCDIR"

AC_CHECK_HEADER(png.h,[],AC_MSG_ERROR(Can't find png.h!))
AC_CHECK_LIB(png, png_read_info)
dnl ****************** End PNG checking ***********************

AC_CHECK_LIB(jpeg, jpeg_CreateCompress)

dnl ****************** Begin GD checking ***********************
if test "x$with_gd" != "x"; then
  GD_DIR=$with_gd
  for i in / /include /include/gd /gd; do
     test -f $with_gd/$i/gd.h           && GD_INCDIR=$with_gd/$i
  done
  test -f $with_gd/lib/libgd.a -o -f $with_gd/lib/libgd.so -o -f $with_gd/lib/libgd.dylib && GD_LIBDIR=$with_gd/lib
  test -f $with_gd/libgd.a -o -f $with_gd/libgd.so -o -f $with_gd/libgd.dylib&& GD_LIBDIR=$with_gd
else
  for i in /usr/include /usr/local/include /usr/local/include/gd; do
    test -f $i/gd.h                       && GD_INCDIR=$i
  done
  for i in /usr /usr/local; do
    test -f $i/lib/libgd.a -o -f $i/lib/libgd.so -o -f $i/lib/libgd.dylib && GD_LIBDIR=$i/lib
    test -f $i/lib/gd/libgd.a -o -f $i/lib/gd/libgd.so -o -f $i/lib/gd/libgd.dylib && GD_LIBDIR=$i/lib/gd
  done
fi

if test "x$GD_INCDIR" == "x"; then
   GD_INCDIR=`pwd`/ext/include
fi
if test "x$GD_LIBDIR" == "x"; then
   GD_LIBDIR=`pwd`/ext/lib
fi

LDFLAGS="-L$GD_LIBDIR $LDFLAGS"
dnl CFLAGS="-I$GD_INCDIR $CFLAGS"
CPPFLAGS="$CPPFLAGS -I$GD_INCDIR"

AC_CHECK_HEADER(gd.h,[],AC_MSG_ERROR(Can't find gd.h!))
PREGDL=$LIBS
AC_CHECK_LIB(gd, gdImageCreateFromPng,[],AC_MSG_ERROR(Can't find libgd!))
dnl ****************** End GD checking ***********************

AC_CHECK_LIB(iconv, libiconv)
AC_CHECK_LIB(freetype, FT_New_Face, [
OLIBS=$LIBS
LIBS="-lgd -lfreetype $PREGDL"
AC_MSG_CHECKING([usability of FT in GD])
AC_LINK_IFELSE([
#include "gd.h"
int main(void) {
    gdImageStringFT(0, 0, 0, "bla", 12.0, 0.0, 10, 10, "foo");
    return 0;
}
],[
AC_DEFINE(HAS_FTL, 1, [is freetype library available?])
AC_MSG_RESULT(looks good)
],[
LIBS="-lgd -lfreetype -lfontconfig $PREGDL"
AC_LINK_IFELSE([
#include "gd.h"
int main(void) {
    gdImageStringFT(0, 0, 0, "bla", 12.0, 0.0, 10, 10, "foo");
    return 0;
}
],[
AC_DEFINE(HAS_FTL, 1, [is freetype library available?])
AC_MSG_RESULT(needs fontconfig, but looks good)
],[
LIBS=$OLIBS
AC_MSG_RESULT(not usable)
])
])
])

fi

AC_MSG_CHECKING([whether GD programs can be compiled])
AC_LINK_IFELSE([
#include "gd.h"
int main(void) {
    gdImagePtr o;
    return 0;
}
	],[AC_MSG_RESULT(yes)],
	[AC_MSG_ERROR([Cannot compile a simple GD program. See config.log for details.])])

CPPFLAGS="$CPPFLAGS -DHAVE_GDDCONFIG_H"

#AC_SUBST(CFLAGS)
#AC_SUBST(CPPFLAGS)
#AC_SUBST(LDFLAGS)

AC_CONFIG_FILES([src/Makevars])
AC_OUTPUT
