/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
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
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "xtypes.h"
#include "xmalloc.h"
#include "error.h"

#ifndef NDEBUG
static int memory_alloc = 0;
#endif

/* Review: look into dmalloc */
#define MALLOC_MAGIC        0xf00fbaab
#define MALLOC_PAD_SIZE     16
#define MALLOC_PAD_FILL     0x55

#ifndef NDEBUG
int xmemory(void)
{
    return memory_alloc;
}

static int _checkfill(char *buf, unsigned char fill, int size)
{
    while (size-- > 0)
        if (buf[size] != fill)
            return 0;
    return 1;
}
#endif

char *xmalloc(int size)
{
    char *new;
    int *p;


    assert(size > 0);
    p = (int *) malloc(2*sizeof(int) + size + MALLOC_PAD_SIZE);
    if (p == NULL)
        err_exit(FALSE, "out of memory");
    p[0] = MALLOC_MAGIC;                           /* magic cookie */
    p[1] = size;                                   /* store size in buffer */
#ifndef NDEBUG
    memory_alloc += size;
#endif
    new = (char *) &p[2];
    memset(new, 0, size);
    memset(new + size, MALLOC_PAD_FILL, MALLOC_PAD_SIZE);
    return new;
}

char *xrealloc(char *item , int newsize)
{
    char *new;
    int *p = (int *)item - 2;
    int oldsize;


    assert(item != NULL);
    assert(newsize > 0);
    assert(p[0] == MALLOC_MAGIC);
    oldsize = p[1];
    assert(_checkfill(item + oldsize, MALLOC_PAD_FILL, MALLOC_PAD_SIZE));
    p = (int *)realloc(p, 2*sizeof(int) + newsize + MALLOC_PAD_SIZE);
    if (p == NULL)
        err_exit(FALSE, "out of memory");
    assert(p[0] == MALLOC_MAGIC);
    p[1] = newsize;
#ifndef NDEBUG
    memory_alloc += (newsize - oldsize);
#endif
    new = (char *) &p[2];
    if (newsize > oldsize)
        memset(new + oldsize, 0, newsize - oldsize);
    memset(new + newsize, MALLOC_PAD_FILL, MALLOC_PAD_SIZE);
    return new;
}

void xfree(void *ptr)
{
    if (ptr != NULL) {
        int *p = (int *) ptr - 2;
        int size;

        assert(p[0] == MALLOC_MAGIC);   /* magic cookie still there? */
        size = p[1];
        assert(_checkfill((char*)ptr + size, MALLOC_PAD_FILL, MALLOC_PAD_SIZE));
        memset(p, 0, 2*sizeof(int) + size + MALLOC_PAD_SIZE);
#ifndef NDEBUG
        memory_alloc -= size;
#endif
        free(p);
    }
}

char *xstrdup(const char *str)
{
    char *cpy;

    cpy = xmalloc(strlen(str) + 1);

    strcpy(cpy, str);
    return cpy;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
