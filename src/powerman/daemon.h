/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_DAEMON_H
#define PM_DAEMON_H

void daemon_init(int *skipfds, int skipfdslen, char *rundir, char *pidfile,
                 char *logname);


#endif /* PM_DAEMON_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
