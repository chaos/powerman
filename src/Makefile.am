##*****************************************************************************
## $Id: Makefile.am,v 1.72 2005/08/25 22:04:32 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

powerman_module_builddir=`cd $(top_builddir)/src && pwd`

if !WITH_STATIC_MODULES
LTDL_LIBS = $(LIBADD_DL)
LTDL_HEADER_FILES = ltdl.h
LTDL_SOURCE_FILES = ltdl.c
endif

noinst_HEADERS = \
	arglist.h \
	argv.h \
	cbuf.h \
	client.h \
        client_proto.h \
	daemon.h \
	debug.h \
	device.h \
	device_pipe.h \
	device_serial.h \
	device_tcp.h \
	error.h \
	hash.h \
	hostlist.h \
	hprintf.h \
	list.h \
	$(LTDL_HEADER_FILES) \
	parse_util.h \
	pluglist.h \
	powerman.h \
	powerman_options.h \
	wrappers.h

COMMON_SOURCE = \
	debug.c \
	error.c \
	hostlist.c \
	list.c \
	wrappers.c

#
# Powerman Modules
#

MODULES_FLAGS = -module -avoid-version

if WITH_GENDERS
GENDERS_OPTIONS_MODULE = powerman_options_genders.la	
GENDERS_OPTIONS_STATIC_SOURCES = powerman_options_genders.c
GENDERS_OPTIONS_STATIC_LIBS = $(GENDERS_LIBS)
endif

if WITH_STATIC_MODULES
noinst_LTLIBRARIES = libstaticmodules.la
STATIC_MODULE_LIBS = libstaticmodules.la
else
powermanmodulelibdir = $(POWERMAN_MODULE_DIR)
powermanmodulelib_LTLIBRARIES = $(GENDERS_OPTIONS_MODULE)
endif

powerman_options_genders_la_SOURCES = powerman_options_genders.c \
				      hostlist.c
powerman_options_genders_la_LDFLAGS = $(MODULES_FLAGS) $(GENDERS_LIBS)

libstaticmodules_la_SOURCES = $(GENDERS_OPTIONS_STATIC_SOURCES)
libstaticmodules_la_LDFLAGS = $(GENDERS_OPTIONS_STATIC_LIBS)

#
# Powerman
#

bin_PROGRAMS = powerman

powerman_SOURCES = \
	powerman.c \
	$(LTDL_SOURCE_FILES) \
	$(COMMON_SOURCE)

powerman_CFLAGS = \
	-DPOWERMAN_VERSION=\"$(VERSION)\" \
	-DPOWERMAN_MODULE_BUILDDIR=\"$(powerman_module_builddir)\"

powerman_LDADD = \
	$(STATIC_MODULE_LIBS)

powerman_LDFLAGS = \
	$(LTDL_LIBS)

#
# Powermand, httppower
#

BUILT_SOURCES = parse_lex.c parse_tab.c

sbin_PROGRAMS = powermand httppower

httppower_SOURCES = \
	httppower.c \
	argv.c

powermand_SOURCES = \
	arglist.c \
	argv.c \
	cbuf.c \
	client.c \
	daemon.c \
	device.c \
	device_pipe.c \
	device_serial.c \
	device_tcp.c \
	hash.c \
	hprintf.c \
	parse_lex.c \
	parse_tab.c \
	parse_util.c \
	pluglist.c \
	powermand.c \
	$(COMMON_SOURCE)

powermand_CFLAGS = \
	-DPOWERMAN_VERSION=\"$(VERSION)\"

powermand_LDADD = \
	$(LIBWRAP) \
	$(LIBFORKPTY)

parse_lex.c: $(srcdir)/parse.lex
	$(LEX) -oparse_lex.c parse.lex

# Including fix for annoying "yyval used uninitialized" warning
parse_tab.c: $(srcdir)/parse.y
	$(YACC) -d -oparse_tab.c parse.y
	sed s/YYSTYPE\ yyval/YYSTYPE\ yyval\ =\ NULL/ $(srcdir)/parse_tab.c > $(srcdir)/foo.c
	mv $(srcdir)/foo.c $(srcdir)/parse_tab.c

CLEANFILES = parse_tab.c parse_tab.h parse_lex.c

EXTRA_DIST = parse.lex parse.y
