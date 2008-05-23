##*****************************************************************************
## $Id: ac_genders.m4,v 1.6 2005/05/10 22:39:40 achu Exp $
##*****************************************************************************

AC_DEFUN([AC_GENDERS],
[
  AC_MSG_CHECKING([for whether to build genders support])
  AC_ARG_WITH([genders],
    AC_HELP_STRING([--with-genders], [Build genders support]),
    [ case "$withval" in
        no)  ac_genders_test=no ;;
        yes) ac_genders_test=yes ;;
        *)   AC_MSG_ERROR([bad value "$withval" for --with-genders]) ;;
      esac
    ]
  )
  AC_MSG_RESULT([${ac_genders_test=no}])

  if test "$ac_genders_test" = "yes"; then
     AC_CHECK_LIB([genders], [genders_handle_create], [ac_have_genders=yes], [])
  fi

  if test "$ac_have_genders" = "yes"; then
     AC_DEFINE([WITH_GENDERS], [1], [Define if you have genders.])
     AC_CHECK_HEADERS( genders.h )
     LIBGENDERS="-lgenders"
     ac_with_genders=yes
  else
     LIBGENDERS=
     ac_with_genders=no
  fi
  AC_SUBST(LIBGENDERS)
])

