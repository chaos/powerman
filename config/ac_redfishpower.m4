#
# Add --without-redfishpower configure option (build by default).
# If yes, not finding curl or jansson is fatal.
#
AC_DEFUN([AC_REDFISHPOWER],
[
  AC_ARG_WITH([redfishpower],
    AS_HELP_STRING([--without-redfishpower], [Do not build redfishpower executable]),
    [],
    [with_redfishpower=yes])
  AS_IF([test "x$with_redfishpower" = "xyes"], [
    AC_CHECK_HEADERS([curl/curl.h])
    X_AC_CHECK_COND_LIB([curl], [curl_multi_perform])
    AC_CHECK_HEADERS([jansson.h])
    X_AC_CHECK_COND_LIB([jansson], [json_object])
  ])
  AS_IF([test "x$with_redfishpower" = "xyes" \
              && test "x$ac_cv_header_curl_curl_h" = "xno" \
                 -o "x$ac_cv_lib_curl_curl_multi_perform" = "xno"], [
    AC_MSG_ERROR([could not find curl library for redfishpower])
  ])
  AS_IF([test "x$with_redfishpower" = "xyes" \
              && test "x$ac_cv_header_jansson_h" = "xno" \
                 -o "x$ac_cv_lib_curl_json_object_set" = "xno"], [
    AC_MSG_ERROR([could not find jansson library for redfishpower])
  ])
  AM_CONDITIONAL(WITH_REDFISHPOWER, [test "x$with_redfishpower" = "xyes"])
])
