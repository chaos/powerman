AC_DEFUN([AC_RUNAS],
[
    RUN_AS_USER="daemon"
    AC_MSG_CHECKING(user to run as)
    AC_ARG_WITH(user,
    AS_HELP_STRING([--with-user=username], [user for powerman daemon (daemon)]),
    [       case "${withval}" in
            yes|no)
                    ;;
            *)
                    RUN_AS_USER="${withval}"
                    ;;
            esac],
    )
    AC_DEFINE_UNQUOTED(RUN_AS_USER, "${RUN_AS_USER}",
            [Powerman daemon user])
    AC_MSG_RESULT(${RUN_AS_USER})
    AC_SUBST(RUN_AS_USER)
])
