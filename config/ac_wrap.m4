#
# Add --with-tcp-wrappers configure option (no by default).
# If yes, not finding tcpd.h header file or libwrap is fatal.
# Define HAVE_TCP_WRAPPERS=1, HAVE_TCP_H=1, and HAVE_LIBWRAP=1 in config.h.
# Substitute LIBWRAP with -lwrap in Makefiles.
#

AC_DEFUN([AC_WRAP],
[
  AC_ARG_WITH([tcp-wrappers],
    AS_HELP_STRING([--with-tcp-wrappers], [Build with tcp wrappers support]))
  AS_IF([test "x$with_tcp_wrappers" = "xyes"], [
    AC_CHECK_HEADERS([tcpd.h])
    X_AC_CHECK_COND_LIB([wrap], [hosts_ctl])
    AC_DEFINE(HAVE_TCP_WRAPPERS, 1, [Define if building tcp wrappers support])
  ])
  AS_IF([test "x$with_tcp_wrappers" = "xyes" && test "x$ac_cv_header_tcpd_h" = "xno" -o "x$ac_cv_lib_wrap_hosts_ctl" = "xno"], [
    AC_MSG_ERROR([could not find tcp wrappers library])
  ])
])
