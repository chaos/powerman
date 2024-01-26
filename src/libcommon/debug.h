/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_DEBUG_H
#define PM_DEBUG_H

#define DBG_ALWAYS          0x0000
#define DBG_DEVICE          0x0001
#define DBG_POLL            0x0002
#define DBG_CLIENT          0x0004
#define DBG_ACTION          0x0008
#define DBG_MEMORY          0x0010
#define DBG_TELNET          0x0020

#define DBG_NAME_TAB {                      \
    { DBG_DEVICE,       "device" },         \
    { DBG_POLL,         "poll"   },         \
    { DBG_CLIENT,       "client" },         \
    { DBG_ACTION,       "action" },         \
    { DBG_MEMORY,       "memory" },         \
    { DBG_TELNET,       "telnet" },         \
    { 0, NULL }                             \
}

void dbg_notty(void);
void dbg_setmask(unsigned long mask);
void dbg_wrapped(unsigned long channel, const char *fmt, ...);
char *dbg_memstr(char *mem, int len);

#define dbg(channel, fmt...)    dbg_wrapped(channel, fmt)

#endif /* PM_DEBUG_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
