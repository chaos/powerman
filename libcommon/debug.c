/*****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-2002-008.
 *  
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see <http://www.llnl.gov/linux/powerman/>.
 *  
 *  PowerMan is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  PowerMan is distributed in the hope that it will be useful, but WITHOUT 
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 *  for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with PowerMan; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif 
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "xtypes.h"
#include "xmalloc.h"
#include "debug.h"

#define DBG_BUFLEN 1024

static unsigned long dbg_channel_mask = 0;
static bool dbg_ttyvalid = TRUE;

void dbg_notty(void)
{
    dbg_ttyvalid = FALSE;
}

void dbg_setmask(unsigned long mask)
{
    dbg_channel_mask = mask;
}

static char *_channel_name(unsigned long channel)
{
    static struct {
        unsigned long chan;
        char *desc;
    } tab[] = DBG_NAME_TAB;
    int i = 0;

    while (tab[i].chan != 0) {
        if (tab[i].chan == channel)
            break;
        i++;
    }
    return (tab[i].chan == 0 ? "<unknown>" : tab[i].desc);
}

static char *_time(void)
{
    time_t now = time(NULL);
    char *str = ctime(&now);

    str[strlen(str) - 1] = '\0'; /* lose trailing \n */

    return str;
}


/*
 * Report message on stdout/syslog if dbg_channel_mask permits.
 */
void dbg_wrapped(unsigned long channel, const char *fmt, ...)
{
    va_list ap;

    if ((channel & dbg_channel_mask) == channel) {
        char buf[DBG_BUFLEN];

        va_start(ap, fmt);
        vsnprintf(buf, DBG_BUFLEN, fmt, ap); /* overflow ignored on purpose */
        va_end(ap);

        if (dbg_ttyvalid)
            fprintf(stderr, "%s %s: %s\n", 
                    _time(), _channel_name(channel), buf);
        else
            syslog(LOG_DEBUG, "%s: %s", 
                    _channel_name(channel), buf);
    }
}

/*
 * Convert memory to string, turning non-printable character into "C" 
 * representation.  
 *  mem (IN) target memory
 *  len (IN) number of characters to convert
 *  RETURN   string (caller must free)
 */
char *dbg_memstr(char *mem, int len)
{
    int i, j;
    int strsize = len * 4;      /* worst case */
    char *str = xmalloc(strsize + 1);

    for (i = j = 0; i < len; i++) {
        switch (mem[i]) {
        case '\r':
            strcpy(&str[j], "\\r");
            j += 2;
            break;
        case '\n':
            strcpy(&str[j], "\\n");
            j += 2;
            break;
        case '\t':
            strcpy(&str[j], "\\t");
            j += 2;
            break;
        default:
            if (isprint(mem[i]))
                str[j++] = mem[i];
            else {
                sprintf(&str[j], "\\%.3o", mem[i]);
                j += 4;
            }
            break;
        }
        assert(j <= strsize || i == len);
    }
    str[j] = '\0';
    return str;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
