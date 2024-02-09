/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_ERROR_H
#define PM_ERROR_H

#include <stdbool.h>
#include <stdarg.h>

void err_init(char *prog);
void err_exit(bool errno_valid, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
void err(bool errno_valid, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

void lsd_fatal_error(char *file, int line, char *mesg);
void *lsd_nomem_error(char *file, int line, char *mesg);

#endif /* PM_ERROR_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
