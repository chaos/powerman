##*****************************************************************************
## $Id: ac_genders.m4,v 1.6 2005/05/10 22:39:40 achu Exp $
##*****************************************************************************

AC_DEFUN([AC_HTTPPOWER],
[
  AC_MSG_CHECKING([for whether to build httppower executable])
  AC_ARG_WITH([httppower],
    AC_HELP_STRING([--with-httppower], [Build httppower executable]),
    [ case "$withval" in
        no)  ac_httppower_test=no ;;
        yes) ac_httppower_test=yes ;;
        *)   AC_MSG_ERROR([bad value "$withval" for --with-httppower]) ;;
      esac
    ]
  )
  AC_MSG_RESULT([${ac_httppower_test=no}])
  
  if test "$ac_httppower_test" = "yes"; then
     ac_with_httppower=yes
  else
     ac_with_httppower=no
  fi
])
