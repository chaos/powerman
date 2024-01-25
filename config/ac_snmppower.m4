#
# Add --with-snmppower configure option (no by default).
# If yes, not finding netsnmp header file or lib is fatal.
# Define HAVE_NET_SNMP_NET_SNMP_CONFIG_H=1 and HAVE_LIBSNMP=1 in config.h.
# Substitute LIBSNMP with -lsnmp and define WITH_SNMPPOWER=1 in Makefiles.
#

AC_DEFUN([AC_SNMPPOWER],
[
  AC_ARG_WITH([snmppower],
    AS_HELP_STRING([--with-snmppower], [Build snmppower executable]))
  AS_IF([test "x$with_snmppower" = "xyes"], [
    AC_CHECK_HEADERS([net-snmp/net-snmp-config.h])
    X_AC_CHECK_COND_LIB([netsnmp], [init_snmp])
  ])
  AS_IF([test "x$with_snmppower" = "xyes" && test "x$ac_cv_header_net_snmp_net_snmp_config_h" = "xno" -o "x$ac_cv_lib_snmp_init_snmp" = "xno"], [
    AC_MSG_ERROR([could not find snmp library])
  ])
  AM_CONDITIONAL(WITH_SNMPPOWER, [test "x$with_snmppower" = "xyes"])
])
