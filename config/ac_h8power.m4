AC_DEFUN([AC_H8POWER],
[
  AC_MSG_CHECKING([for whether to build h8power executable])
  AC_ARG_WITH([h8power],
    AC_HELP_STRING([--with-h8power], [Build h8power executable]),
    [ case "$withval" in
        no)  ac_h8power_test=no ;;
        yes) ac_h8power_test=yes ;;
        *)   AC_MSG_ERROR([bad value "$withval" for --with-h8power]) ;;
      esac
    ]
  )
  AC_MSG_RESULT([${ac_h8power_test=no}])
  
  if test "$ac_h8power_test" = "yes"; then
     ac_with_h8power=yes
  else
     ac_with_h8power=no
  fi
])
