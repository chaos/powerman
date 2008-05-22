##*****************************************************************************
## $Id: configure.ac,v 1.40 2006/09/27 00:44:07 achu Exp $
##*****************************************************************************
## Process this file with autoconf to produce a configure script.
##*****************************************************************************

##
# Prologue.
##
AC_INIT([powerman])
AC_META
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([NEWS])
AC_CANONICAL_SYSTEM

##
# Automake support.
##
AM_INIT_AUTOMAKE($PROJECT, $VERSION)
AM_CONFIG_HEADER([config/config.h])
AM_MAINTAINER_MODE

##
# Checks for programs.
##
AC_PROG_CC
if test "$GCC" = yes; then
  GCCWARN="-Wall"
  AC_SUBST([GCCWARN])
fi
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_LEX
if test "$LEX" != "flex"; then
  AC_ERROR([lex won't cut it, I need flex])
fi
AC_PROG_YACC
AM_CONDITIONAL(WITH_GNU_LD, test "$with_gnu_ld" = "yes")

##
# Checks for header files.
##
AC_HEADER_STDC
AC_CHECK_HEADERS( \
  getopt.h \
)

##
# Checks for typedefs, structures, and compiler characteristics.
##
AC_C_BIGENDIAN
AC_TYPE_UID_T
AC_C_CONST

##
# Check for httppower
##
AC_HTTPPOWER
AM_CONDITIONAL(WITH_HTTPPOWER, [test "$ac_with_httppower" = "yes"])

##
# Checks for library functions.
##
AC_CHECK_FUNCS( \
  getopt_long \
)
AC_SEARCH_LIBS([bind],[socket])
AC_SEARCH_LIBS([gethostbyaddr],[nsl])
AC_FORKPTY
AC_WRAP
AC_CHECK_FUNC([poll], AC_DEFINE([HAVE_POLL], [1], [Define if you have poll]))
AC_READLINE
AC_CURL

# for list.c, cbuf.c, hostlist.c, and wrappers.c */
AC_DEFINE(WITH_LSD_FATAL_ERROR_FUNC, 1, [Define lsd_fatal_error])
AC_DEFINE(WITH_LSD_NOMEM_ERROR_FUNC, 1, [Define lsd_fatal_error])

##
# Epilogue.
##
AC_CONFIG_FILES( \
  Makefile \
  src/Makefile \
  etc/Makefile \
  man/Makefile \
  scripts/Makefile \
)
AC_OUTPUT

