##*****************************************************************************
## $Id: ac_ncurses.m4,v 1.1.1.1 2003/09/05 16:05:42 achu Exp $
##*****************************************************************************
#  AUTHOR:
#    Jim Garlick <garlick@llnl.gov>
#
#  SYNOPSIS:
#    AC_CURL
#
#  DESCRIPTION:
#    Check for curl library
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC or equivalent.
##*****************************************************************************

AC_DEFUN([AC_CURL],
[
  AC_CHECK_LIB([curl], [curl_easy_setopt], [ac_have_curl=yes], [ac_have_curl=no])

  if test "$ac_have_curl" = "yes"; then
    LIBCURL="-lcurl"
    AC_DEFINE([HAVE_CURL], [1], [Define if you have libcurl])
  fi        

  AC_SUBST(LIBCURL)
])
