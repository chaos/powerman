/* config/config.h.  Generated from config.h.in by configure.  */
/* config/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to 1 if you have the `cfmakeraw' function. */
#define HAVE_CFMAKERAW 1

/* Define if you have libcurl */
#define HAVE_CURL 1

/* Define if you have curses.h */
#define HAVE_CURSES_H /**/

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define if you have forkpty */
#define HAVE_FORKPTY 1

/* Define to 1 if you have the <genders.h> header file. */
/* #undef HAVE_GENDERS_H */

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `getopt_long' function. */
#define HAVE_GETOPT_LONG 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `genders' library (-lgenders). */
/* #undef HAVE_LIBGENDERS */

/* Define to 1 if you have the `lua' library (-llua). */
#define HAVE_LIBLUA 1

/* Define to 1 if you have the `munge' library (-lmunge). */
#define HAVE_LIBMUNGE 1

/* Define to 1 if you have the `snmp' library (-lsnmp). */
#define HAVE_LIBSNMP 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define if you have ncurses.h */
/* #undef HAVE_NCURSES_H */

/* Define if you have poll */
#define HAVE_POLL 1

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* Define to 1 if you have the <pty.h> header file. */
#define HAVE_PTY_H 1

/* Define to 1 if the system has the type `socklen_t'. */
#define HAVE_SOCKLEN_T 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/syscall.h> header file. */
#define HAVE_SYS_SYSCALL_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define if you have tcp wrappers */
#define HAVE_TCP_WRAPPERS 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <util.h> header file. */
/* #undef HAVE_UTIL_H */

/* Define to 1 if you have the file `AC_File'. */
/* #undef HAVE__DEV_PTC */

/* Define to 1 if you have the file `AC_File'. */
#define HAVE__DEV_PTMX 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define the project alias string (name-ver or name-ver-rel). */
#define META_ALIAS "powerman-2.3.11"

/* Define the project author. */
/* #undef META_AUTHOR */

/* Define the project release date. */
#define META_DATE "2011-08-31"

/* Define the libtool library 'age' version information. */
/* #undef META_LT_AGE */

/* Define the libtool library 'current' version information. */
/* #undef META_LT_CURRENT */

/* Define the libtool library 'revision' version information. */
/* #undef META_LT_REVISION */

/* Define the project name. */
#define META_NAME "powerman"

/* Define the project release. */
#define META_RELEASE "1"

/* Define the project version. */
#define META_VERSION "2.3.11"

/* Name of package */
#define PACKAGE "powerman"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "powerman"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "powerman 2.3.11"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "powerman"

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.3.11"

/* Powerman daemon user */
#define RUN_AS_USER "daemon"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "2.3.11"

/* Define if building genders support for client */
/* #undef WITH_GENDERS */

/* Define lsd_fatal_error */
#define WITH_LSD_FATAL_ERROR_FUNC 1

/* Define lsd_fatal_error */
#define WITH_LSD_NOMEM_ERROR_FUNC 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Expansion of the "bindir" installation directory. */
#define X_BINDIR "/usr/local/bin"

/* Expansion of the "datadir" installation directory. */
#define X_DATADIR "${prefix}/share"

/* Expansion of the "exec_prefix" installation directory. */
#define X_EXEC_PREFIX "/usr/local"

/* Expansion of the "includedir" installation directory. */
#define X_INCLUDEDIR "/usr/local/include"

/* Expansion of the "infodir" installation directory. */
#define X_INFODIR "${prefix}/share/info"

/* Expansion of the "libdir" installation directory. */
#define X_LIBDIR "/usr/local/lib"

/* Expansion of the "libexecdir" installation directory. */
#define X_LIBEXECDIR "/usr/local/libexec"

/* Expansion of the "localstatedir" installation directory. */
#define X_LOCALSTATEDIR "/usr/local/var"

/* Expansion of the "mandir" installation directory. */
#define X_MANDIR "${prefix}/share/man"

/* Expansion of the "oldincludedir" installation directory. */
#define X_OLDINCLUDEDIR "/usr/include"

/* Expansion of the "prefix" installation directory. */
#define X_PREFIX "/usr/local"

/* Expansion of the "sbindir" installation directory. */
#define X_SBINDIR "/usr/local/sbin"

/* Expansion of the "sharedstatedir" installation directory. */
#define X_SHAREDSTATEDIR "/usr/local/com"

/* Expansion of the "sysconfdir" installation directory. */
#define X_SYSCONFDIR "/usr/local/etc"

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#define YYTEXT_POINTER 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */
