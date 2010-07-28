dnl
dnl $ Id: $
dnl

PHP_ARG_WITH(epeg, [whether to enable Epeg support],
[  --with-epeg[[=PATH]]      Include Epeg support.
                          PATH is the optional pathname to epeg-config.
                          If epeg-config does not exist, use pkg-config], yes, yes)

if test "$PHP_EPEG" != "no"; then

  if test -z "$AWK"; then
    AC_PATH_PROGS(AWK, awk gawk nawk, [no])
  fi
  if test -z "$SED"; then
    AC_PATH_PROGS(SED, sed gsed, [no])
  fi
  if test -z "$PKG_CONFIG"; then
    AC_PATH_PROGS(PKG_CONFIG, pkg-config, [no])
  fi

  dnl
  dnl Check the location of epeg-config
  dnl
  EPEG_CONFIG=""
  if test "$PHP_EPEG" != "yes"; then
    AC_MSG_CHECKING([for epeg-config])
    if test -f "$PHP_EPEG"; then
      EPEG_CONFIG="$PHP_EPEG"
    elif test -f "$PHP_EPEG/epeg-config"; then
      EPEG_CONFIG="$PHP_EPEG/epeg-config"
    elif test -f "$PHP_EPEG/bin/epeg-config"; then
      EPEG_CONFIG="$PHP_EPEG/bin/epeg-config"
    fi
    if test -z "$EPEG_CONFIG"; then
      AC_MSG_RESULT([not found, use pkg-config])
    else
      AC_MSG_RESULT([$EPEG_CONFIG])
    fi
  else
    AC_PATH_PROGS(EPEG_CONFIG, epeg-config, [])
    if test -z "$EPEG_CONFIG"; then
      AC_MSG_RESULT([epeg-config not found, use pkg-config])
    fi
  fi

  dnl
  dnl Get the version number, CFLAGS and LIBS by epeg-config
  dnl
  AC_MSG_CHECKING([for Epeg library version])
  if test -z "$EPEG_CONFIG"; then
    EPEG_VERSION=`$PKG_CONFIG --modversion epeg 2> /dev/null`
    EPEG_INCLINE=`$PKG_CONFIG --cflags epeg 2> /dev/null`
    EPEG_LIBLINE=`$PKG_CONFIG --libs epeg 2> /dev/null`
  else
    EPEG_VERSION=`$EPEG_CONFIG --version 2> /dev/null`
    EPEG_INCLINE=`$EPEG_CONFIG --cflags 2> /dev/null`
    EPEG_LIBLINE=`$EPEG_CONFIG --libs 2> /dev/null`
  fi

  if test -z "$EPEG_VERSION"; then
    if test -z "$EPEG_CONFIG"; then
      AC_MSG_ERROR([pkg-config is not found in PATH or epeg.pc is not found in PKG_CONFIG_PATH!])
    else
      AC_MSG_ERROR([invalid epeg-config passed to --with-epeg!])
    fi
  fi

  EPEG_VERSION_NUMBER=`echo $EPEG_VERSION | $AWK -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3 }'`

  if test "$EPEG_VERSION_NUMBER" -lt 9000 -o "$EPEG_VERSION_NUMBER" -ge 10000 ; then
    AC_MSG_RESULT([$EPEG_VERSION])
    AC_MSG_ERROR([Epeg version 0.9.x is required to compile php with Epeg support.])
  fi

  AC_DEFINE_UNQUOTED(PHP_EPEG_VERSION_STRING, "$EPEG_VERSION", [Epeg library version])
  AC_MSG_RESULT([$EPEG_VERSION (ok)])

  dnl
  dnl Check the headers and types
  dnl
  export OLD_CPPFLAGS="$CPPFLAGS"
  export CPPFLAGS="$CPPFLAGS $EPEG_INCLINE"
  AC_CHECK_HEADER([Epeg.h], [], AC_MSG_ERROR([Epeg.h header not found.]))
  export CPPFLAGS="$OLD_CPPFLAGS"

  PHP_EVAL_INCLINE($EPEG_INCLINE)

  dnl
  dnl Check the library
  dnl
  PHP_CHECK_LIBRARY(epeg, epeg_file_open,
    [
      PHP_EVAL_LIBLINE($EPEG_LIBLINE, EPEG_SHARED_LIBADD)
    ],[
      AC_MSG_ERROR([wrong Epeg library version or lib not found. Check config.log for more information.])
    ],[
      $EPEG_LIBLINE
    ])

  PHP_ADD_LIBRARY(m, 1, EPEG_SHARED_LIBADD)
  PHP_SUBST(EPEG_SHARED_LIBADD)
  PHP_NEW_EXTENSION(epeg, epeg.c , $ext_shared)

fi
