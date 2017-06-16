AC_DEFUN([AC_RUNAS],
[
    RUN_AS_USER="daemon"
    RUN_AS_GROUP="daemon"
    AC_MSG_CHECKING(user to run as)
    AC_ARG_WITH(user,
    AC_HELP_STRING([--with-user=username], [user for powerman daemon (daemon)]),
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

    AC_MSG_CHECKING(group to run as)
    AC_ARG_WITH(group,
    AC_HELP_STRING([--with-group=groupname], [group for powerman daemon (daemon)]),
    [       case "${withval}" in
            yes|no)
                    ;;
            *)
                    RUN_AS_GROUP="${withval}"
                    ;;
            esac],
    )
    AC_DEFINE_UNQUOTED(RUN_AS_GROUP, "${RUN_AS_GROUP}",
            [Powerman daemon group])
    AC_MSG_RESULT(${RUN_AS_GROUP})
    AC_SUBST(RUN_AS_GROUP)
])
