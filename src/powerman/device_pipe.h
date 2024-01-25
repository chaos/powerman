/************************************************************\
 * Copyright (C) 2003 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_DEVICE_PIPE_H
#define PM_DEVICE_PIPE_H

bool pipe_connect(Device * dev);
void pipe_disconnect(Device * dev);
void *pipe_create(char *cmdline, char *flags);
void pipe_destroy(void *data);

#endif /* PM_DEVICE_PIPE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
