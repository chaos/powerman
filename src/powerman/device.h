/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_DEVICE_H
#define PM_DEVICE_H

void dev_init(bool short_circuit_delay);
void dev_fini(void);
void dev_initial_connect(void);

void dev_pre_poll(xpollfd_t pfd);
void dev_post_poll(xpollfd_t pfd, struct timeval *tv);

#endif /* PM_DEVICE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
