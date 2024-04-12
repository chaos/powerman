#
# Add --without-httppower configure option (build by default).
# If yes, not finding curl header file or lib is fatal.
# Define HAVE_CURL_CURL_H=1, and HAVE_LIBCURL=1 in config.h.
# Substitute LIBCURL with -lcurl and define WITH_HTTPPOWER=1 in Makefiles.
#
#
AC_DEFUN([AC_HTTPPOWER],
[
  AC_ARG_WITH([httppower],
    AS_HELP_STRING([--without-httppower], [Do not build httppower executable]),
    [],
    [with_httppower=yes])
  AS_IF([test "x$with_httppower" = "xyes"], [
    AC_CHECK_HEADERS([curl/curl.h])
    X_AC_CHECK_COND_LIB([curl], [curl_easy_setopt])
  ])
  AS_IF([test "x$with_httppower" = "xyes" && test "x$ac_cv_header_curl_curl_h" = "xno" -o "x$ac_cv_lib_curl_curl_easy_setopt" = "xno"], [
    AC_MSG_ERROR([could not find curl library for httppower])
  ])
  AM_CONDITIONAL(WITH_HTTPPOWER, [test "x$with_httppower" = "xyes"])
])
