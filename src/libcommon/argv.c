/************************************************************\
 * Copyright (C) 2003 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "xmalloc.h"
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
        cpy = (char *)xmalloc(len + 1);
        memcpy(cpy, word, len);
        cpy[len] = '\0';
    }

    *strp = str;
    return cpy;
}

/* return number of space separated words in str */
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

int argv_length(char **argv)
{
    int i = 0;

    while (argv[i] != NULL)
        i++;

    return i;
}

char **argv_append(char **argv, char *s)
{
    int argc = argv_length(argv) + 1;

    argv = (char **)xrealloc((char *)argv, sizeof(char *) * (argc + 1));
    argv[argc - 1] = xstrdup(s);
    argv[argc] = NULL;

    return argv;
}

/* Create a null-terminated argv array given a command line.
 * Characters in the 'ignore' set are treated like white space.
 */
char **argv_create(char *cmdline, char *ignore)
{
    int argc = _sizeargv(cmdline, ignore);
    char **argv = (char **)xmalloc(sizeof(char *) * (argc + 1));
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
        xfree((void *)argv[i]);
    xfree((void *)argv);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
