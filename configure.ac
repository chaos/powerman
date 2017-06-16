##
# Prologue.
##
AC_INIT([powerman],
        m4_esyscmd([git describe --always | awk '/.*/ {printf "%s",$1; exit}']))
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([NEWS])
AC_CANONICAL_SYSTEM
X_AC_EXPAND_INSTALL_DIRS

##
# Automake support.
##
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])
AM_CONFIG_HEADER([config/config.h])
AM_MAINTAINER_MODE

##
# Checks for programs.
##
AC_PROG_CC
if test "$GCC" = yes; then
  #GCCWARN="-Wall -Werror"
  GCCWARN="-Wall"
  AC_SUBST([GCCWARN])
fi
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_LIBTOOL
AM_CONDITIONAL(WITH_GNU_LD, test "$with_gnu_ld" = "yes")

##
#
##
AC_CHECK_FILES(/dev/ptmx)
AC_CHECK_FILES(/dev/ptc)

##
# Checks for header files.
##
AC_HEADER_STDC
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
AC_CURSES
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
  libpowerman/Makefile \
  libpowerman/libpowerman.pc \
  liblsd/Makefile \
  libcommon/Makefile \
  powerman/Makefile \
  powermand/Makefile \
  httppower/Makefile \
  snmppower/Makefile \
  plmpower/Makefile \
  etc/Makefile \
  scripts/Makefile \
  scripts/powerman \
  scripts/powerman.service \
  scripts/tmpfiles.d/powerman.conf \
  heartbeat/Makefile \
  man/Makefile \
  man/powerman.1 \
  man/libpowerman.3 \
  man/powerman.conf.5 \
  man/powerman.dev.5 \
  man/httppower.8 \
  man/plmpower.8 \
  man/powermand.8 \
  man/vpcd.8 \
  test/Makefile \
  test/mcr.conf
  test/sierra.conf \
  test/t07.conf \
  test/t09.conf \
  test/t10.conf \
  test/t17.conf \
  test/t18.conf \
  test/t19.conf \
  test/t20.conf \
  test/t21.conf \
  test/t22.conf \
  test/t23.conf \
  test/t24.conf \
  test/t25.conf \
  test/t26.conf \
  test/t27.conf \
  test/t28.conf \
  test/t29.conf \
  test/t30.conf \
  test/t31.conf \
  test/t32.conf \
  test/t33.conf \
  test/t34.conf \
  test/t35.conf \
  test/t36.conf \
  test/t37.conf \
  test/t38.conf \
  test/t39.conf \
  test/t40.conf \
  test/t41.conf \
  test/t42.conf \
  test/t43.conf \
  test/t44.conf \
  test/t45.conf \
  test/t46.conf \
  test/t47.conf \
  test/t48.conf \
  test/t49.conf \
  test/t50.conf \
  test/t51.conf \
  test/t53.conf \
  test/t54.conf \
  test/t55.conf \
  test/t60.conf \
  test/test.conf \
  test/test4.conf \
)

AC_OUTPUT

