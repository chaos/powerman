/*****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
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

#include "xtypes.h"
#include "error.h"
#include "xregex.h"
#include "xmalloc.h"

#define XREGEX_MAGIC 0x3456aaaa
struct xregex_struct {
    int         xr_magic;
    int         xr_cflags;
    regex_t    *xr_regex; 
};
#define XREGEX_MATCH_MAGIC 0x3456aaba
struct xregex_match_struct {
    int         xm_magic;
    int         xm_nmatch;
    regmatch_t *xm_pmatch;
    char       *xm_str;
    int         xm_result;
    bool        xm_used;
};

xregex_t 
xregex_create(void)
{
    xregex_t xrp = (xregex_t)xmalloc(sizeof(struct xregex_struct));

    xrp->xr_magic = XREGEX_MAGIC;
    xrp->xr_regex = NULL;

    return xrp;
}

void 
xregex_destroy(xregex_t xrp)
{
    assert(xrp->xr_magic == XREGEX_MAGIC);
    if (xrp->xr_regex) {
        regfree(xrp->xr_regex);
        xfree(xrp->xr_regex);
    }
    xrp->xr_magic = 0;
    xfree(xrp);
}

/* Substitute all occurrences of s2 with s3 in s1, 
 * e.g. _str_subst(str, "\\r", "\r") 
 */
static void 
_str_subst(char *s1, int len, const char *s2, const char *s3)
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

void 
xregex_compile(xregex_t xrp, const char *regex, bool withsub)
{
    char tmpstr[256];
    char *cpy;
    int n;

    assert(regex != NULL);
    assert(xrp->xr_magic == XREGEX_MAGIC);
    assert(xrp->xr_regex == NULL);

/* No particular limit is imposed  on  the  length  of  REs(!).   Programs
 * intended to be portable should not employ REs longer than 256 bytes, as
 * an implementation can refuse to accept such REs and  remain  POSIX-com-
 * pliant.  -- regex(7) on RHEL 5
 */
    if (strlen(regex) > 256)
        err_exit(FALSE, "refusing to compile regex > 256 bytes");

    xrp->xr_regex = (regex_t *)xmalloc(sizeof(regex_t));
    xrp->xr_cflags = REG_EXTENDED;
    if (!withsub)
        xrp->xr_cflags |= REG_NOSUB;

    cpy = xstrdup(regex);
    _str_subst(cpy, strlen(cpy) + 1, "\\r", "\r");
    _str_subst(cpy, strlen(cpy) + 1, "\\n", "\n");
    n = regcomp(xrp->xr_regex, cpy, xrp->xr_cflags);
    xfree(cpy);

    if (n != 0) {
        regerror(n, xrp->xr_regex, tmpstr, sizeof(tmpstr));
        err_exit(FALSE, "regcomp failed: %s", tmpstr);
    }
}

bool 
xregex_exec(xregex_t xrp, const char *s, xregex_match_t xm)
{
    int eflags = REG_NOTEOL;
    int res;

    assert(xrp->xr_magic == XREGEX_MAGIC);
    assert(xrp->xr_regex != NULL);
    if (xm != NULL) {
        assert(xm->xm_magic == XREGEX_MATCH_MAGIC);
        assert(xm->xm_used == FALSE);
    }

    res = regexec(xrp->xr_regex, s, xm ? xm->xm_nmatch : 0, 
                                    xm ? xm->xm_pmatch : NULL, eflags);
    if (xm != NULL) {
        xm->xm_result = res;
        xm->xm_used = TRUE;
        if (res == 0) {
            if (xm->xm_str)
                xfree(xm->xm_str);
            xm->xm_str = xstrdup(s);
        }
    }
    return res == 0 ? TRUE : FALSE;
}

xregex_match_t 
xregex_match_create(int nmatch)
{
    xregex_match_t xm;

    xm = (xregex_match_t)xmalloc(sizeof(struct xregex_match_struct));
    xm->xm_magic = XREGEX_MATCH_MAGIC;
    xm->xm_nmatch = nmatch + 1;
    xm->xm_pmatch = (regmatch_t *)xmalloc(sizeof(regmatch_t) * (nmatch + 1));
    xm->xm_str = NULL;
    xm->xm_result = -1;
    xm->xm_used = FALSE;
    return xm;
}

void 
xregex_match_destroy(xregex_match_t xm)
{
    assert(xm->xm_magic == XREGEX_MATCH_MAGIC);

    xfree(xm->xm_pmatch);
    if (xm->xm_str)
        xfree(xm->xm_str);
    xm->xm_magic = 0;
    xfree(xm);
}

void
xregex_match_recycle(xregex_match_t xm)
{
    if (xm->xm_str) {
        xfree(xm->xm_str);
        xm->xm_str = NULL;
    }
    xm->xm_result = -1;
    xm->xm_used = FALSE;
}

char *
xregex_match_strdup(xregex_match_t xm)
{
    char *s = NULL;

    assert(xm->xm_magic == XREGEX_MATCH_MAGIC);
    assert(xm->xm_used);
  
    if (xm->xm_result == 0) {
        s = (char *)xmalloc(xm->xm_pmatch[0].rm_eo + 1);
        assert(xm->xm_str != NULL);
        memcpy(s, xm->xm_str, xm->xm_pmatch[0].rm_eo);
        s[xm->xm_pmatch[0].rm_eo] = '\0';
    }
    return s;
}

int
xregex_match_strlen(xregex_match_t xm)
{
    assert(xm->xm_magic == XREGEX_MATCH_MAGIC);
    assert(xm->xm_used);

    return xm->xm_pmatch[0].rm_eo;
}

char *
xregex_match_sub_strdup(xregex_match_t xm, int i)
{
    char *s = NULL;

    assert(xm->xm_magic == XREGEX_MATCH_MAGIC);
    assert(xm->xm_used);
  
    if (xm->xm_result == 0 && i >= 0 && i < xm->xm_nmatch 
                           && xm->xm_pmatch[i].rm_so != -1) {
        regmatch_t m = xm->xm_pmatch[i];

        assert(xm->xm_str != NULL);
        assert(m.rm_so < m.rm_eo);
        s = xmalloc(m.rm_eo - m.rm_so + 1);
        memcpy(s, xm->xm_str + m.rm_so, m.rm_eo - m.rm_so);
        s[m.rm_eo - m.rm_so] = '\0';
    }
    return s;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
