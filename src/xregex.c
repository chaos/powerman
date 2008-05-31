/*****************************************************************************\
 *  $Id: wrappers.c 911 2008-05-30 20:26:33Z garlick $
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
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
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>

#include "error.h"
#include "xregex.h"

#define MAX_REG_BUF 64000

/* 
 * Substitute all occurrences of s2 with s3 in s1, 
 * e.g. _str_subst(str, "\\r", "\r") 
 */
static void _str_subst(char *s1, int len, const char *s2, const char *s3)
{
    int s2len = strlen(s2);
    int s3len = strlen(s3);
    char *p;

    while ((p = strstr(s1, s2)) != NULL) {
        assert(strlen(s1) + (s3len - s2len) + 1 <= len);
        memmove(p + s3len, p + s2len, strlen(p + s2len) + 1);
        memcpy(p, s3, s3len);
    }
}

#ifndef REG_NOERROR
#define REG_NOERROR 0
#endif

void xregcomp(regex_t * preg, const char *regex, int cflags)
{
    char buf[MAX_REG_BUF];
    int n;

    assert(regex != NULL);
    assert(strlen(regex) < sizeof(buf));

    snprintf(buf, sizeof(buf), "%s", regex);

    /* convert backslash-prefixed special characters in regex to value */
    _str_subst(buf, MAX_REG_BUF, "\\r", "\r");
    _str_subst(buf, MAX_REG_BUF, "\\n", "\n");

    /*
     * N.B.
     * The buffer space available in a compiled RegEx expression is only 
     * 256 bytes.  A long or complicated RegEx will exceed this space and 
     * cause the library call to silently fail.
     */
    n = regcomp(preg, buf, cflags);
    if (n != REG_NOERROR)
        err_exit(FALSE, "regcomp failed");
}

int
xregexec(const regex_t * preg, const char *string,
        size_t nmatch, regmatch_t pmatch[], int eflags)
{
    int n;
    char buf[MAX_REG_BUF];

    snprintf(buf, sizeof(buf), "%s", string);
    n = regexec(preg, buf, nmatch, pmatch, eflags);
    return n;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
