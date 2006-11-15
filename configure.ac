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
AC_PROG_LN_S
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AM_PROG_LEX
AC_PROG_YACC
AM_CONDITIONAL(WITH_GNU_LD, test "$with_gnu_ld" = "yes")
AC_DEBUG
AC_LIB_LTDL

##
# Checks for header files.
##
AC_HEADER_STDC
AC_CHECK_HEADERS( \
  fcntl.h \
  unistd.h \
  getopt.h \
  string.h \
)

##
# Checks for typedefs, structures, and compiler characteristics.
##
AC_C_BIGENDIAN
AC_C_CONST
AC_TYPE_UID_T

##
# Check for static module compilation
##
AC_STATIC_MODULES
AM_CONDITIONAL(WITH_STATIC_MODULES, [test "$ac_with_static_modules" = "yes"])

##
# Module Directory
##
AC_POWERMAN_MODULE_DIR

##
# Check for genders
##
AC_GENDERS
AM_CONDITIONAL(WITH_GENDERS, [test "$ac_with_genders" = "yes"])

##
# Checks for library functions.
##
AC_CHECK_FUNCS( \
  strcmp \
  strlen \
  strcpy \
  strchr \
  strsep \
  strspn \
  strtok_r \
  getopt_long \
)
AC_FORKPTY
AC_WRAP
AC_CHECK_FUNC([poll], AC_DEFINE([HAVE_POLL], [1], [Define that you will use poll()]))

# for list.c, cbuf.c, hostlist.c, and wrappers.c */
AC_DEFINE(WITH_LSD_FATAL_ERROR_FUNC, 1, [Define lsd_fatal_error])
AC_DEFINE(WITH_LSD_NOMEM_ERROR_FUNC, 1, [Define lsd_fatal_error])

##
# Epilogue.
##
AC_CONFIG_FILES( \
  powerman.spec \
  Makefile \
  etc/Makefile \
  man/Makefile \
  man/powerman.1 \
  scripts/Makefile \
  src/Makefile \
)
AC_OUTPUT

