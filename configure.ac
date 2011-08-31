##
# Prologue.
##
AC_INIT(m4_esyscmd([awk '/Name:/ {printf "%s",$2; exit}' META]),
        m4_esyscmd([awk '/Version:/ {printf "%s",$2; exit}' META]))
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([NEWS])
AC_CANONICAL_SYSTEM
X_AC_META
X_AC_EXPAND_INSTALL_DIRS

##
# Automake support.
##
AM_INIT_AUTOMAKE([$META_NAME], [$META_VERSION])
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
X_AC_CHECK_COND_LIB(lua, luaL_newstate)
X_AC_CHECK_COND_LIB(munge, munge_ctx_create)

# for list.c, cbuf.c, hostlist.c, and wrappers.c */
AC_DEFINE(WITH_LSD_FATAL_ERROR_FUNC, 1, [Define lsd_fatal_error])
AC_DEFINE(WITH_LSD_NOMEM_ERROR_FUNC, 1, [Define lsd_fatal_error])

# whether to install pkg-config file for API
AC_PKGCONFIG

# what user to run daemon as
AC_RUNAS

##
# Epilogue.
##
AC_CONFIG_FILES( \
  Makefile \
  powerman.spec \
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
  heartbeat/Makefile \
  test/Makefile \
  man/Makefile \
  man/powerman.1 \
  man/libpowerman.3 \
  man/powerman.conf.5 \
  man/powerman.dev.5 \
  man/httppower.8 \
  man/plmpower.8 \
  man/powermand.8 \
  man/vpcd.8 \
)

AC_OUTPUT

