/************************************************************\
 * Copyright (C) 2004 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_XSIGNAL_H
#define PM_XSIGNAL_H

typedef void xsigfunc_t(int);
xsigfunc_t *xsignal(int signo, xsigfunc_t * func);

#endif /* PM_XSIGNAL_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
