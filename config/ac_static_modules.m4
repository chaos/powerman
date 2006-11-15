##*****************************************************************************
## $Id: ac_static_modules.m4,v 1.4 2005/08/23 21:10:14 achu Exp $
##*****************************************************************************

AC_DEFUN([AC_STATIC_MODULES],
[
  AC_MSG_CHECKING([for static module compilation])
  AC_ARG_ENABLE([static-modules],
    AC_HELP_STRING([--enable-static-modules], [enable build with static modules]),
    [ case "$enableval" in
        yes) ac_with_static_modules=yes ;;
        no)  ac_with_static_modules=no ;;
        *)   AC_MSG_ERROR([bad value "$enableval" for --enable-static-modules]) ;;
      esac
    ]
  )
  AC_MSG_RESULT([${ac_with_static_modules=no}])

  if test "$ac_with_static_modules" = "yes"; then
     AC_DEFINE([WITH_STATIC_MODULES], [1], [Define if builing with static modules])
     MANPAGE_STATIC_MODULES=1
  else 
     MANPAGE_STATIC_MODULES=0
  fi

  AC_SUBST(WITH_STATIC_MODULES)
  AC_SUBST(MANPAGE_STATIC_MODULES)
])
