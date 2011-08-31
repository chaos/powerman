#
# Add --with-genders configure option (no by default).
# If yes, not finding header file or lib is fatal.
# Define WITH_GENDERS=1, HAVE_GENDERS_H=1, and HAVE_LIBGENDERS=1 in config.h.
# Substitute LIBGENDERS with -lgenders in Makefiles.
#
AC_DEFUN([AC_GENDERS],
[
  AC_ARG_WITH([genders],
    AC_HELP_STRING([--with-genders], [Build genders support for client]))
  AS_IF([test "x$with_genders" = "xyes"], [
    AC_CHECK_HEADERS([genders.h])
    X_AC_CHECK_COND_LIB([genders], [genders_handle_create])
    AC_DEFINE(WITH_GENDERS, 1, [Define if building genders support for client])
  ])
  AS_IF([test "x$with_genders" = "xyes" && test "x$ac_cv_header_genders_h" = "xno" -o "x$ac_cv_lib_genders_genders_handle_create" = "xno"], [
    AC_MSG_ERROR([could not find genders library])
  ])
])

