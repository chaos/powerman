##
# Prologue.
##
AC_INIT([powerman],
	m4_esyscmd([git describe --always | awk '/.*/ {sub(/^v/, ""); printf "%s",$1; exit}']))
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AC_CONFIG_SRCDIR([NEWS])
AC_CANONICAL_TARGET

LT_INIT

##
# If runstatedir not explicitly set on command line, use '/run' as default
# N.B. runstatedir is not set at all in autoconf < 2.70.
##
if test "$runstatedir" = '${localstatedir}/run' || test -z "$runstatedir"; then
   AC_SUBST([runstatedir],[/run])
fi
X_AC_EXPAND_INSTALL_DIRS

##
# Automake support.
##
AM_INIT_AUTOMAKE([subdir-objects foreign])
AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([config/config.h])
AM_MAINTAINER_MODE([enable])

##
# Checks for programs.
##
AC_PROG_CC
AX_COMPILER_VENDOR
AS_CASE($ax_cv_c_compiler_vendor,
  [gnu], [
      WARNING_CFLAGS="-Wall -Werror"
  ]
  [clang], [
      WARNING_CFLAGS="-Wall -Werror -Wno-unknown-warning-option -Wno-error=unknown-warning-option"
  ]
)
AC_SUBST([WARNING_CFLAGS])

AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LEX([noyywrap])
AS_IF([test "x$LEX" = "x:"],AC_MSG_ERROR([could not find flex]),[])
AC_PROG_YACC
# AC_PROG_YACC sets YACC=yacc when nothing is found, even yacc
# Just assume bison is the preferred compiler-compiler and call yacc a failure
AS_IF([test "x$YACC" = "xyacc"],AC_MSG_ERROR([could not find bison]),[])
AM_CONDITIONAL(WITH_GNU_LD, test "$with_gnu_ld" = "yes")

##
#
##
AC_CHECK_FILES(/dev/ptmx)
AC_CHECK_FILES(/dev/ptc)

##
# Checks for header files.
##
AC_CHECK_HEADERS( \
  getopt.h \
  poll.h \
  sys/select.h \
  sys/syscall.h \
)

##
# Checks for typedefs, structures, and compiler characteristics.
##
AC_C_BIGENDIAN
AC_TYPE_UID_T
AC_C_CONST
AC_CHECK_TYPES(socklen_t, [], [], [#include <sys/socket.h>])

##
# Check for httppower, genders
##
AC_HTTPPOWER
AC_REDFISHPOWER
AC_SNMPPOWER
AC_GENDERS

##
# Check for systemd
##
RRA_WITH_SYSTEMD_UNITDIR

##
# Checks for library functions.
##
AC_CHECK_FUNCS( \
  getopt_long \
  cfmakeraw
)
AC_SEARCH_LIBS([bind],[socket])
AC_SEARCH_LIBS([gethostbyaddr],[nsl])
AC_FORKPTY
AC_WRAP
AC_CHECK_FUNC([poll], AC_DEFINE([HAVE_POLL], [1], [Define if you have poll]))

# for list.c, cbuf.c, hostlist.c, and wrappers.c */
AC_DEFINE(WITH_LSD_FATAL_ERROR_FUNC, 1, [Define lsd_fatal_error])
AC_DEFINE(WITH_LSD_NOMEM_ERROR_FUNC, 1, [Define lsd_fatal_error])

# whether to install pkg-config file for API
AC_PKGCONFIG

# what user and group to run daemon as
AC_RUNAS

##
# Epilogue.
##
AC_CONFIG_FILES( \
  Makefile \
  examples/powerman_el72.spec \
  src/Makefile \
  src/liblsd/Makefile \
  src/libcommon/Makefile \
  src/powerman/Makefile \
  src/httppower/Makefile \
  src/redfishpower/Makefile \
  src/snmppower/Makefile \
  src/plmpower/Makefile \
  src/libtap/Makefile \
  etc/Makefile \
  etc/libpowerman.pc \
  etc/powerman.service \
  etc/tmpfiles.d/powerman.conf \
  heartbeat/Makefile \
  man/Makefile \
  man/powerman.1 \
  man/libpowerman.3 \
  man/powerman.conf.5 \
  man/powerman.dev.5 \
  man/httppower.8 \
  man/redfishpower.8 \
  man/plmpower.8 \
  man/powermand.8 \
  t/Makefile \
  test/Makefile \
  test/mcr.conf
  test/sierra.conf \
  test/t53.conf \
  test/t54.conf \
  test/t55.conf \
  test/t60.conf \
  test/t62.conf \
  test/t63.conf \
  test/test.conf \
  test/test4.conf \
)

AC_OUTPUT

