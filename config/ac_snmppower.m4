AC_DEFUN([AC_SNMPPOWER],
[
  AC_MSG_CHECKING([for whether to build snmppower executable])
  AC_ARG_WITH([snmppower],
    AC_HELP_STRING([--with-snmppower], [Build snmppower executable]),
    [ case "$withval" in
        no)  ac_snmppower_test=no ;;
        yes) ac_snmppower_test=yes ;;
        *)   AC_MSG_ERROR([bad value "$withval" for --with-snmppower]) ;;
      esac
    ]
  )
  AC_MSG_RESULT([${ac_snmppower_test=no}])
  
  if test "$ac_snmppower_test" = "yes"; then
     ac_with_snmppower=yes
  else
     ac_with_snmppower=no
  fi
])
