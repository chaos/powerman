##*****************************************************************************
## $Id: ac_ncurses.m4,v 1.1.1.1 2003/09/05 16:05:42 achu Exp $
##*****************************************************************************
#  AUTHOR:
#    Jim Garlick <garlick@llnl.gov>
#
#  SYNOPSIS:
#    AC_READLINE
#
#  DESCRIPTION:
#    Check for readline library
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC or equivalent.
##*****************************************************************************

AC_DEFUN([AC_READLINE],
[
  AC_CHECK_LIB([readline], [readline], 
     [ac_have_readline=yes], [ac_have_readline=no], [-lcurses])

  if test "$ac_have_readline" = "yes"; then
    LIBREADLINE="-lreadline -lcurses"
  else
    AC_MSG_ERROR([libreadline required for this package])
  fi        

  AC_SUBST(LIBREADLINE)
])
