/************************************************************\
 * Copyright (C) 2003 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_DEVICE_SERIAL_H
#define PM_DEVICE_SERIAL_H

bool serial_connect(Device * dev);
void serial_disconnect(Device * dev);
void *serial_create(char *special, char *flags);
void serial_destroy(void *data);

#endif /* PM_DEVICE_SERIAL_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
