/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_XMALLOC_H
#define PM_XMALLOC_H

char *xmalloc(int size);
char *xrealloc(char *item, int newsize);
void xfree(void *ptr);
char *xstrdup(const char *str);
int xmemory(void);

#endif /* PM_XMALLOC_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
