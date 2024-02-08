/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_FDUTIL_H
#define PM_FDUTIL_H

void nonblock_set(int fd);
void nonblock_clr(int fd);

#endif /* PM_FDUTIL_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
