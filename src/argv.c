/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2003 The Regents of the University of California.
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

#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "wrappers.h"
#include "argv.h"

/* make a copy of the first word in str and advance str past it */
static char *_nextargv(char **strp, char *ignore)
{
    char *str = *strp;
    char *word; 
    int len;
    char *cpy = NULL;

    while (*str && (isspace(*str) || strchr(ignore, *str)))
        str++;
    word = str;
    while (*str && !(isspace(*str) || strchr(ignore, *str)))
        str++;
    len = str - word;

    if (len > 0) {
        cpy = (char *)Malloc(len + 1);
        memcpy(cpy, word, len);
        cpy[len] = '\0';
    }

    *strp = str;
    return cpy;
}

/* return number of space seperated words in str */
static int _sizeargv(char *str, char *ignore)
{
    int count = 0;

    do {
        while (*str && (isspace(*str) || strchr(ignore, *str)))
            str++;
        if (*str)
            count++;
        while (*str && !(isspace(*str) || strchr(ignore, *str)))
            str++;
    } while (*str);

    return count;
}

/* Create a null-terminated argv array given a command line.
 * Characters in the 'ignore' set are treated like white space. 
 */
char **argv_create(char *cmdline, char *ignore)
{
    int argc = _sizeargv(cmdline, ignore);
    char **argv = (char **)Malloc(sizeof(char *) * (argc + 1));
    int i;

    for (i = 0; i < argc; i++) {
        argv[i] = _nextargv(&cmdline, ignore);
        assert(argv[i] != NULL);
    }
    argv[i] = NULL;

    return argv;
}

/* Destroy a null-terminated argv array.
 */
void argv_destroy(char **argv)
{
    int i;

    for (i = 0; argv[i] != NULL; i++)
        Free((void *)argv[i]);
    Free((void *)argv);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
