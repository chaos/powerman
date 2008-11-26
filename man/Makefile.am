##*****************************************************************************
## $Id: Makefile.am,v 1.32 2005/07/26 00:57:17 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

man1_MANS = powerman.1 pm.1

man5_MANS = powerman.conf.5 powerman.dev.5

man7_MANS = powerman-devices.7

man8_MANS = powermand.8 httppower.8 plmpower.8

pm.1: powerman.1
	cp powerman.1 $@

CLEANFILES = pm.1
	
EXTRA_DIST = \
	powerman.1 \
	powerman.conf.5 \
	powerman.dev.5 \
	powerman-devices.7 \
	powermand.8 \
	httppower.8 \
	plmpower.8
