/*****************************************************************************
 *  $LSDId: cbuf.c,v 1.13 2002/11/07 19:50:37 dun Exp $
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Chris Dunlap <cdunlap@llnl.gov>.
 *
 *  This file is from LSD-Tools, the LLNL Software Development Toolbox.
 *
 *  LSD-Tools is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  LSD-Tools is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with LSD-Tools; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 *****************************************************************************
 *  Refer to "cbuf.h" for documentation on public functions.
 *****************************************************************************/


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef WITH_PTHREADS
#  include <pthread.h>
#endif /* WITH_PTHREADS */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cbuf.h"


/*********************
 *  lsd_fatal_error  *
 *********************/

#ifdef WITH_LSD_FATAL_ERROR_FUNC
#  undef lsd_fatal_error
   extern void lsd_fatal_error(char *file, int line, char *mesg);
#else /* !WITH_LSD_FATAL_ERROR_FUNC */
#  ifndef lsd_fatal_error
#    include <errno.h>
#    include <stdio.h>
#    include <string.h>
#    define lsd_fatal_error(file, line, mesg)                                 \
       do {                                                                   \
           fprintf(stderr, "ERROR: [%s:%d] %s: %s\n",                         \
                   file, line, mesg, strerror(errno));                        \
       } while (0)
#  endif /* !lsd_fatal_error */
#endif /* !WITH_LSD_FATAL_ERROR_FUNC */


/*********************
 *  lsd_nomem_error  *
 *********************/

#ifdef WITH_LSD_NOMEM_ERROR_FUNC
#  undef lsd_nomem_error
   extern void * lsd_nomem_error(char *file, int line, char *mesg);
#else /* !WITH_LSD_NOMEM_ERROR_FUNC */
#  ifndef lsd_nomem_error
#    define lsd_nomem_error(file, line, mesg) (NULL)
#  endif /* !lsd_nomem_error */
#endif /* !WITH_LSD_NOMEM_ERROR_FUNC */


/***************
 *  Constants  *
 ***************/

#define CBUF_CHUNK      500
#define CBUF_MAGIC      0xDEADBEEF
#define CBUF_MAGIC_LEN  (sizeof(unsigned long))


/****************
 *  Data Types  *
 ****************/

struct cbuf {

#ifndef NDEBUG
    unsigned long       magic;          /* cookie for asserting validity     */
#endif /* !NDEBUG */

#ifdef WITH_PTHREADS
    pthread_mutex_t     mutex;          /* mutex to protect access to cbuf   */
#endif /* WITH_PTHREADS */

    int                 alloc;          /* num bytes malloc'd/realloc'd      */
    int                 minsize;        /* min bytes of data to allocate     */
    int                 maxsize;        /* max bytes of data to allocate     */
    int                 size;           /* num bytes of data allocated       */
    int                 used;           /* num bytes of unread data          */
    cbuf_overwrite_t    overwrite;      /* overwrite option behavior         */
    int                 i_in;           /* index to where data is written in */
    int                 i_out;          /* index to where data is read out   */
    unsigned char      *data;           /* ptr to circular buffer of data    */
};

typedef int (*cbuf_iof) (void *cbuf_data, void *arg, int len);


/****************
 *  Prototypes  *
 ****************/

static int cbuf_find_line (cbuf_t cb);

static int cbuf_get_fd (void *dstbuf, int *psrcfd, int len);
static int cbuf_get_mem (void *dstbuf, unsigned char **psrcbuf, int len);
static int cbuf_put_fd (void *srcbuf, int *pdstfd, int len);
static int cbuf_put_mem (void *srcbuf, unsigned char **pdstbuf, int len);

static int cbuf_copier (cbuf_t src, cbuf_t dst, int len, int *ndropped);
static int cbuf_dropper (cbuf_t cb, int len);
static int cbuf_reader (cbuf_t src, int len, cbuf_iof putf, void *dst);
static int cbuf_replayer (cbuf_t src, int len, cbuf_iof putf, void *dst);
static int cbuf_writer (cbuf_t dst, int len, cbuf_iof getf, void *src,
       int *ndropped);

static int cbuf_grow (cbuf_t cb, int n);
static int cbuf_shrink (cbuf_t cb);

#ifndef NDEBUG
static int cbuf_is_valid (cbuf_t cb);
#endif /* !NDEBUG */


/************
 *  Macros  *
 ************/

#ifndef MAX
#  define MAX(x,y) (((x) >= (y)) ? (x) : (y))
#endif /* !MAX */

#ifndef MIN
#  define MIN(x,y) (((x) <= (y)) ? (x) : (y))
#endif /* !MIN */

#ifdef WITH_PTHREADS

#  define cbuf_mutex_init(cb)                                                 \
     do {                                                                     \
         int e = pthread_mutex_init(&cb->mutex, NULL);                        \
         if (e) {                                                             \
             errno = e;                                                       \
             lsd_fatal_error(__FILE__, __LINE__, "cbuf mutex init");          \
             abort();                                                         \
         }                                                                    \
     } while (0)

#  define cbuf_mutex_lock(cb)                                                 \
     do {                                                                     \
         int e = pthread_mutex_lock(&cb->mutex);                              \
         if (e) {                                                             \
             errno = e;                                                       \
             lsd_fatal_error(__FILE__, __LINE__, "cbuf mutex lock");          \
             abort();                                                         \
         }                                                                    \
     } while (0)

#  define cbuf_mutex_unlock(cb)                                               \
     do {                                                                     \
         int e = pthread_mutex_unlock(&cb->mutex);                            \
         if (e) {                                                             \
             errno = e;                                                       \
             lsd_fatal_error(__FILE__, __LINE__, "cbuf mutex unlock");        \
             abort();                                                         \
         }                                                                    \
     } while (0)

#  define cbuf_mutex_destroy(cb)                                              \
     do {                                                                     \
         int e = pthread_mutex_destroy(&cb->mutex);                           \
         if (e) {                                                             \
             errno = e;                                                       \
             lsd_fatal_error(__FILE__, __LINE__, "cbuf mutex destroy");       \
             abort();                                                         \
         }                                                                    \
     } while (0)

#  ifndef NDEBUG
     static int cbuf_mutex_is_locked (cbuf_t cb);
#  endif /* !NDEBUG */

#else /* !WITH_PTHREADS */

#  define cbuf_mutex_init(cb)
#  define cbuf_mutex_lock(cb)
#  define cbuf_mutex_unlock(cb)
#  define cbuf_mutex_destroy(cb)
#  define cbuf_mutex_is_locked(cb) (1)

#endif /* !WITH_PTHREADS */


/***************
 *  Functions  *
 ***************/

cbuf_t
cbuf_create (int minsize, int maxsize)
{
    cbuf_t cb;

    if (minsize <= 0) {
        errno = EINVAL;
        return(NULL);
    }
    if (!(cb = malloc(sizeof(struct cbuf)))) {
        errno = ENOMEM;
        return(lsd_nomem_error(__FILE__, __LINE__, "cbuf struct"));
    }
    /*  Circular buffer is empty when (i_in == i_out),
     *    so reserve 1 byte for this sentinel.
     */
    cb->alloc = minsize + 1;
#ifndef NDEBUG
    /*  Reserve space for the magic cookies used to protect the
     *    cbuf data[] array from underflow and overflow.
     */
    cb->alloc += 2 * CBUF_MAGIC_LEN;
#endif /* !NDEBUG */

    if (!(cb->data = malloc(cb->alloc))) {
        free(cb);
        errno = ENOMEM;
        return(lsd_nomem_error(__FILE__, __LINE__, "cbuf data"));
    }
    cbuf_mutex_init(cb);
    cb->minsize = minsize;
    cb->maxsize = (maxsize > minsize) ? maxsize : minsize;
    cb->size = minsize;
    cb->used = 0;
    cb->overwrite = CBUF_WRAP_MANY;
    cb->i_in = cb->i_out = 0;

#ifndef NDEBUG
    /*  C is for cookie, that's good enough for me, yeah!
     *  The magic cookies are only defined during DEBUG code.
     *  The first "magic" cookie is at the top of the structure.
     *  Magic cookies are also placed at the top & bottom of the
     *  cbuf data[] array to catch buffer underflow & overflow errors.
     */
    cb->data += CBUF_MAGIC_LEN;         /* jump forward past underflow magic */
    cb->magic = CBUF_MAGIC;
    /*
     *  Must use memcpy since overflow cookie may not be word-aligned.
     */
    memcpy(cb->data - CBUF_MAGIC_LEN, (void *) &cb->magic, CBUF_MAGIC_LEN);
    memcpy(cb->data + cb->size + 1, (void *) &cb->magic, CBUF_MAGIC_LEN);

    cbuf_mutex_lock(cb);
    assert(cbuf_is_valid(cb));
    cbuf_mutex_unlock(cb);
#endif /* !NDEBUG */

    return(cb);
}


void
cbuf_destroy (cbuf_t cb)
{
    assert(cb != NULL);
    cbuf_mutex_lock(cb);
    assert(cbuf_is_valid(cb));

#ifndef NDEBUG
    /*  The moon sometimes looks like a C, but you can't eat that.
     *  Munch the magic cookies before freeing memory.
     */
    cb->magic = ~CBUF_MAGIC;            /* the anti-cookie! */
    memcpy(cb->data - CBUF_MAGIC_LEN, (void *) &cb->magic, CBUF_MAGIC_LEN);
    memcpy(cb->data + cb->size + 1, (void *) &cb->magic, CBUF_MAGIC_LEN);
    cb->data -= CBUF_MAGIC_LEN;         /* jump back to what malloc returned */
#endif /* !NDEBUG */

    free(cb->data);
    cbuf_mutex_unlock(cb);
    cbuf_mutex_destroy(cb);
    free(cb);
    return;
}


int
cbuf_opt_get (cbuf_t cb, cbuf_opt_t name, int *value)
{
    int rc = 0;

    assert(cb != NULL);

    if (value == NULL) {
        errno = EINVAL;
        return(-1);
    }
    cbuf_mutex_lock(cb);
    assert(cbuf_is_valid(cb));
    if (name == CBUF_OPT_OVERWRITE) {
        *value = cb->overwrite;
    }
    else {
        errno = EINVAL;
        rc = -1;
    }
    cbuf_mutex_unlock(cb);
    return(rc);
}


int
cbuf_opt_set (cbuf_t cb, cbuf_opt_t name, int value)
{
    int rc = 0;

    assert(cb != NULL);

    cbuf_mutex_lock(cb);
    assert(cbuf_is_valid(cb));
    if (name == CBUF_OPT_OVERWRITE) {
        if  (  (value == CBUF_NO_DROP)
            || (value == CBUF_WRAP_ONCE)
            || (value == CBUF_WRAP_MANY) ) {
            cb->overwrite = value;
        }
        else {
            errno = EINVAL;
            rc = -1;
        }
    }
    else {
        errno = EINVAL;
        rc = -1;
    }
    assert(cbuf_is_valid(cb));
    cbuf_mutex_unlock(cb);
    return(rc);
}


void
cbuf_flush (cbuf_t cb)
{
    assert(cb != NULL);
    cbuf_mutex_lock(cb);
    assert(cbuf_is_valid(cb));
    cb->used = 0;
    cb->i_in = cb->i_out = 0;
    assert(cbuf_is_valid(cb));
    cbuf_mutex_unlock(cb);
    return;
}


int
cbuf_is_empty (cbuf_t cb)
{
    int used;

    assert(cb != NULL);
    cbuf_mutex_lock(cb);
    assert(cbuf_is_valid(cb));
    used = cb->used;
    cbuf_mutex_unlock(cb);
    return(used == 0);
}


int
cbuf_size (cbuf_t cb)
{
    int size;

    assert(cb != NULL);
    cbuf_mutex_lock(cb);
    assert(cbuf_is_valid(cb));
    size = cb->size;
    cbuf_mutex_unlock(cb);
    return(size);
}


int
cbuf_free (cbuf_t cb)
{
    int free;

    assert(cb != NULL);
    cbuf_mutex_lock(cb);
    assert(cbuf_is_valid(cb));
    free = cb->size - cb->used;
    cbuf_mutex_unlock(cb);
    return(free);
}


int
cbuf_used (cbuf_t cb)
{
    int used;

    assert(cb != NULL);
    cbuf_mutex_lock(cb);
    assert(cbuf_is_valid(cb));
    used = cb->used;
    cbuf_mutex_unlock(cb);
    return(used);
}


int
cbuf_drop (cbuf_t cb, int len)
{
    int n;

    assert(cb != NULL);

    if (len < 0) {
        errno = EINVAL;
        return(-1);
    }
    if (len == 0) {
        return(0);
    }
    cbuf_mutex_lock(cb);
    assert(cbuf_is_valid(cb));
    n = MIN(len, cb->used);
    cbuf_dropper(cb, n);
    assert(cbuf_is_valid(cb));
    cbuf_mutex_unlock(cb);
    return(n);
}


int
cbuf_copy (cbuf_t src, cbuf_t dst, int len, int *ndropped)
{
    int n;

    assert(src != NULL);
    assert(dst != NULL);

    if (src == dst) {
        errno = EINVAL;
        return(-1);
    }
    if (len < -1) {
        errno = EINVAL;
        return(-1);
    }
    if (len == 0) {
        return(0);
    }
    /*  Lock cbufs in order of lowest memory address to prevent deadlock.
     */
    if (src < dst) {
        cbuf_mutex_lock(src);
        cbuf_mutex_lock(dst);
    }
    else {
        cbuf_mutex_lock(dst);
        cbuf_mutex_lock(src);
    }
    assert(cbuf_is_valid(src));
    assert(cbuf_is_valid(dst));

    if (len == -1) {
        len = src->used;
    }
    if (len > 0) {
        n = cbuf_copier(src, dst, len, ndropped);
    }
    assert(cbuf_is_valid(src));
    assert(cbuf_is_valid(dst));
    cbuf_mutex_unlock(src);
    cbuf_mutex_unlock(dst);
    return(n);
}


int
cbuf_move (cbuf_t src, cbuf_t dst, int len, int *ndropped)
{
    int n;

    assert(src != NULL);
    assert(dst != NULL);

    if (src == dst) {
        errno = EINVAL;
        return(-1);
    }
    if (len < -1) {
        errno = EINVAL;
        return(-1);
    }
    if (len == 0) {
        return(0);
    }
    /*  Lock cbufs in order of lowest memory address to prevent deadlock.
     */
    if (src < dst) {
        cbuf_mutex_lock(src);
        cbuf_mutex_lock(dst);
    }
    else {
        cbuf_mutex_lock(dst);
        cbuf_mutex_lock(src);
    }
    assert(cbuf_is_valid(src));
    assert(cbuf_is_valid(dst));

    if (len == -1) {
        len = src->used;
    }
    if (len > 0) {
        n = cbuf_copier(src, dst, len, ndropped);
        if (n > 0) {
            cbuf_dropper(src, n);
        }
    }
    assert(cbuf_is_valid(src));
    assert(cbuf_is_valid(dst));
    cbuf_mutex_unlock(src);
    cbuf_mutex_unlock(dst);
    return(n);
}


int
cbuf_peek (cbuf_t src, void *dstbuf, int len)
{
    int n;

    assert(src != NULL);

    if ((dstbuf == NULL) || (len < 0)) {
        errno = EINVAL;
        return(-1);
    }
    if (len == 0) {
        return(0);
    }
    cbuf_mutex_lock(src);
    assert(cbuf_is_valid(src));
    n = cbuf_reader(src, len, (cbuf_iof) cbuf_put_mem, &dstbuf);
    assert(cbuf_is_valid(src));
    cbuf_mutex_unlock(src);
    return(n);
}


int
cbuf_read (cbuf_t src, void *dstbuf, int len)
{
    int n;

    assert(src != NULL);

    if ((dstbuf == NULL) || (len < 0)) {
        errno = EINVAL;
        return(-1);
    }
    if (len == 0) {
        return(0);
    }
    cbuf_mutex_lock(src);
    assert(cbuf_is_valid(src));
    n = cbuf_reader(src, len, (cbuf_iof) cbuf_put_mem, &dstbuf);
    if (n > 0) {
        cbuf_dropper(src, n);
    }
    assert(cbuf_is_valid(src));
    cbuf_mutex_unlock(src);
    return(n);
}


int
cbuf_replay (cbuf_t src, void *dstbuf, int len)
{
    int n;

    assert(src != NULL);

    if ((dstbuf == NULL) || (len < 0)) {
        errno = EINVAL;
        return(-1);
    }
    if (len == 0) {
        return(0);
    }
    cbuf_mutex_lock(src);
    assert(cbuf_is_valid(src));
    n = cbuf_replayer(src, len, (cbuf_iof) cbuf_put_mem, &dstbuf);
    assert(cbuf_is_valid(src));
    cbuf_mutex_unlock(src);
    return(n);
}


int
cbuf_write (cbuf_t dst, void *srcbuf, int len, int *ndropped)
{
    int n;

    assert(dst != NULL);

    if ((srcbuf == NULL) || (len < 0)) {
        errno = EINVAL;
        return(-1);
    }
    if (ndropped) {
        *ndropped = 0;
    }
    if (len == 0) {
        return(0);
    }
    cbuf_mutex_lock(dst);
    assert(cbuf_is_valid(dst));
    n = cbuf_writer(dst, len, (cbuf_iof) cbuf_get_mem, &srcbuf, ndropped);
    assert(cbuf_is_valid(dst));
    cbuf_mutex_unlock(dst);
    return(n);
}


int
cbuf_peek_to_fd (cbuf_t src, int dstfd, int len)
{
    int n = 0;

    assert(src != NULL);

    if ((dstfd < 0) || (len < -1)) {
        errno = EINVAL;
        return(-1);
    }
    cbuf_mutex_lock(src);
    assert(cbuf_is_valid(src));
    if (len == -1) {
        len = src->used;
    }
    if (len > 0) {
        n = cbuf_reader(src, len, (cbuf_iof) cbuf_put_fd, &dstfd);
    }
    assert(cbuf_is_valid(src));
    cbuf_mutex_unlock(src);
    return(n);
}


int
cbuf_read_to_fd (cbuf_t src, int dstfd, int len)
{
    int n = 0;

    assert(src != NULL);

    if ((dstfd < 0) || (len < -1)) {
        errno = EINVAL;
        return(-1);
    }
    cbuf_mutex_lock(src);
    assert(cbuf_is_valid(src));
    if (len == -1) {
        len = src->used;
    }
    if (len > 0) {
        n = cbuf_reader(src, len, (cbuf_iof) cbuf_put_fd, &dstfd);
        if (n > 0) {
            cbuf_dropper(src, n);
        }
    }
    assert(cbuf_is_valid(src));
    cbuf_mutex_unlock(src);
    return(n);
}


int
cbuf_replay_to_fd (cbuf_t src, int dstfd, int len)
{
    int n = 0;

    assert(src != NULL);

    if ((dstfd < 0) || (len < -1)) {
        errno = EINVAL;
        return(-1);
    }
    cbuf_mutex_lock(src);
    assert(cbuf_is_valid(src));
    if (len == -1) {
        len = src->size - src->used;
    }
    if (len > 0) {
        n = cbuf_replayer(src, len, (cbuf_iof) cbuf_put_fd, &dstfd);
    }
    assert(cbuf_is_valid(src));
    cbuf_mutex_unlock(src);
    return(n);
}


int
cbuf_write_from_fd (cbuf_t dst, int srcfd, int len, int *ndropped)
{
    int n = 0;

    assert(dst != NULL);

    if ((srcfd < 0) || (len < -1)) {
        errno = EINVAL;
        return(-1);
    }
    if (ndropped) {
        *ndropped = 0;
    }
    cbuf_mutex_lock(dst);
    assert(cbuf_is_valid(dst));
    if (len == -1) {
        /*
         *  Try to use all of the free buffer space available for writing.
         *    If it is all in use, try to grab another chunk and limit the
         *    amount of data being overwritten.
         */
        len = dst->size - dst->used;
        if (len == 0) {
            len = MIN(dst->size, CBUF_CHUNK);
        }
    }
    if (len > 0) {
        n = cbuf_writer(dst, len, (cbuf_iof) cbuf_get_fd, &srcfd, ndropped);
    }
    assert(cbuf_is_valid(dst));
    cbuf_mutex_unlock(dst);
    return(n);
}


int
cbuf_get_line (cbuf_t src, char *dstbuf, int len)
{
    int n, m;
    char *pdst;

    assert(src != NULL);

    if (((dstbuf == NULL) && (len != 0)) || (len < 0)) {
        errno = EINVAL;
        return(-1);
    }
    cbuf_mutex_lock(src);
    assert(cbuf_is_valid(src));
    n = cbuf_find_line(src);
    if (n > 0) {
        if (len > 0) {
            pdst = dstbuf;
            m = MIN(n, len - 1);
            if (m > 0) {
                n = cbuf_reader(src, m, (cbuf_iof) cbuf_put_mem, &pdst);
                assert(n == m);
            }
            dstbuf[m] = '\0';
        }
        cbuf_dropper(src, n);
    }
    assert(cbuf_is_valid(src));
    cbuf_mutex_unlock(src);
    return(n);
}


int
cbuf_peek_line (cbuf_t src, char *dstbuf, int len)
{
    int n, m;
    char *pdst;

    assert(src != NULL);

    if (((dstbuf == NULL) && (len != 0)) || (len < 0)) {
        errno = EINVAL;
        return(-1);
    }
    cbuf_mutex_lock(src);
    assert(cbuf_is_valid(src));
    n = cbuf_find_line(src);
    if (n > 0) {
        if (len > 0) {
            pdst = dstbuf;
            m = MIN(n, len - 1);
            if (m > 0) {
                n = cbuf_reader(src, m, (cbuf_iof) cbuf_put_mem, &pdst);
                assert(n == m);
            }
            dstbuf[m] = '\0';
        }
    }
    assert(cbuf_is_valid(src));
    cbuf_mutex_unlock(src);
    return(n);
}


int
cbuf_put_line (cbuf_t dst, char *srcbuf, int *ndropped)
{
    int len;
    int nfree, ncopy, n;
    int ndrop = 0, d;
    char *psrc = srcbuf;
    char *newline = "\n";

    assert(dst != NULL);

    if (srcbuf == NULL) {
        errno = EINVAL;
        return(-1);
    }
    /*  Compute number of bytes to effectively copy to dst cbuf.
     *  Reserve space for the trailing newline if needed.
     */
    len = ncopy = strlen(srcbuf);
    if ((len == 0) || (srcbuf[len - 1] != '\n')) {
        len++;
    }
    cbuf_mutex_lock(dst);
    assert(cbuf_is_valid(dst));
    /*
     *  Attempt to grow dst cbuf if necessary.
     */
    nfree = dst->size - dst->used;
    if ((len > nfree) && (dst->size < dst->maxsize)) {
        nfree += cbuf_grow(dst, len - nfree);
    }
    /*  Determine if src will fit (or be made to fit) in dst cbuf.
     */
    if (dst->overwrite == CBUF_NO_DROP) {
        if (len > dst->size - dst->used) {
            errno = ENOSPC;
            len = -1;
        }
    }
    else if (dst->overwrite == CBUF_WRAP_ONCE) {
        if (len > dst->size) {
            errno = ENOSPC;
            len = -1;
        }
    }
    if (len > 0) {
        /*
         *  Discard data that won't fit in dst cbuf.
         */
        if (len > dst->size) {
            ndrop += len - dst->size;
            ncopy -= ndrop;
            psrc += ndrop;
        }
        /*  Copy data from src string to dst cbuf.
         */
        if (ncopy > 0) {
            n = cbuf_writer(dst, ncopy, (cbuf_iof) cbuf_get_mem, &psrc, &d);
            assert(n == ncopy);
            ndrop += d;
        }
        /*  Append newline if needed.
         */
        if (srcbuf[len - 1] != '\n') {
            n = cbuf_writer(dst, 1, (cbuf_iof) cbuf_get_mem, &newline, &d);
            assert(n == 1);
            ndrop += d;
        }
    }
    assert(cbuf_is_valid(dst));
    cbuf_mutex_unlock(dst);
    if (ndropped) {
        *ndropped = ndrop;
    }
    return(len);
}


static int
cbuf_find_line (cbuf_t cb)
{
/*  Returns the number of bytes up to and including the next newline or NUL,
 *    or 0 if a newline/NUL is not found.
 */
    int i, n;
    unsigned char c;

    assert(cb != NULL);
    assert(cbuf_mutex_is_locked(cb));

    i = cb->i_out;
    n = 1;
    while (i != cb->i_in) {
        c = cb->data[i];
        if ((c == '\n') || (c == '\0')) {
            assert(n <= cb->used);
            return(n);
        }
        i = (i + 1) % (cb->size + 1);
        n++;
    }
    return(0);
}


static int
cbuf_get_fd (void *dstbuf, int *psrcfd, int len)
{
/*  Copies data from the file referenced by the file descriptor
 *    pointed at by [psrcfd] into cbuf's [dstbuf].
 *  Returns the number of bytes read from the fd, 0 on EOF, or -1 on error.
 */
    int n;

    assert(dstbuf != NULL);
    assert(psrcfd != NULL);
    assert(*psrcfd >= 0);
    assert(len > 0);

    do {
        n = read(*psrcfd, dstbuf, len);
    } while ((n < 0) && (errno == EINTR));
    return(n);
}


static int
cbuf_get_mem (void *dstbuf, unsigned char **psrcbuf, int len)
{
/*  Copies data from the buffer pointed at by [psrcbuf] into cbuf's [dstbuf].
 *  Returns the number of bytes copied.
 */
    assert(dstbuf != NULL);
    assert(psrcbuf != NULL);
    assert(*psrcbuf != NULL);
    assert(len > 0);

    memcpy(dstbuf, *psrcbuf, len);
    *psrcbuf += len;
    return(len);
}


static int
cbuf_put_fd (void *srcbuf, int *pdstfd, int len)
{
/*  Copies data from cbuf's [srcbuf] into the file referenced
 *    by the file descriptor pointed at by [pdstfd].
 *  Returns the number of bytes written to the fd, or -1 on error.
 */
    int n;

    assert(srcbuf != NULL);
    assert(pdstfd != NULL);
    assert(*pdstfd >= 0);
    assert(len > 0);

    do {
        n = write(*pdstfd, srcbuf, len);
    } while ((n < 0) && (errno == EINTR));
    return(n);
}


static int
cbuf_put_mem (void *srcbuf, unsigned char **pdstbuf, int len)
{
/*  Copies data from cbuf's [srcbuf] into the buffer pointed at by [pdstbuf].
 *  Returns the number of bytes copied.
 */
    assert(srcbuf != NULL);
    assert(pdstbuf != NULL);
    assert(*pdstbuf != NULL);
    assert(len > 0);

    memcpy(*pdstbuf, srcbuf, len);
    *pdstbuf += len;
    return(len);
}


static int
cbuf_copier (cbuf_t src, cbuf_t dst, int len, int *ndropped)
{
/*  Copies up to [len] bytes from the [src] cbuf into the [dst] cbuf.
 *  Returns the number of bytes copied, or -1 on error (with errno set).
 *  Sets [ndropped] (if not NULL) to the number of [dst] bytes overwritten.
 */
    int nfree, ncopy, nleft, n;
    int i_src, i_dst;

    assert(src != NULL);
    assert(dst != NULL);
    assert(len > 0);
    assert(cbuf_mutex_is_locked(src));
    assert(cbuf_mutex_is_locked(dst));

    /*  Bound len by the number of bytes available.
     */
    len = MIN(len, src->used);
    /*
     *  Attempt to grow dst cbuf if necessary.
     */
    nfree = dst->size - dst->used;
    if ((len > nfree) && (dst->size < dst->maxsize)) {
        nfree += cbuf_grow(dst, len - nfree);
    }
    /*  Compute number of bytes to effectively copy to dst cbuf.
     */
    if (dst->overwrite == CBUF_NO_DROP) {
        len = MIN(len, dst->size - dst->used);
        if (len == 0) {
            errno = ENOSPC;
            len = -1;
        }
    }
    else if (dst->overwrite == CBUF_WRAP_ONCE) {
        len = MIN(len, dst->size);
    }
    /*  Compute number of bytes that will be overwritten in dst cbuf.
     */
    if (ndropped) {
        *ndropped = MAX(0, len - dst->size + dst->used);
    }
    /*  Compute number of bytes to physically copy to dst cbuf.  This prevents
     *    copying data that will overwritten if the cbuf wraps multiple times.
     */
    ncopy = len;
    i_src = src->i_out;
    i_dst = dst->i_in;
    if (ncopy > dst->size) {
        n = ncopy - dst->size;
        i_src = (i_src + n) % (src->size + 1);
        ncopy -= n;
    }
    /*  Copy data from src cbuf to dst cbuf.
     */
    nleft = ncopy;
    while (nleft > 0) {
        n = MIN(((src->size + 1) - i_src), ((dst->size + 1) - i_dst));
        n = MIN(n, nleft);
        memcpy(&dst->data[i_dst], &src->data[i_src], n);
        i_src = (i_src + n) % (src->size + 1);
        i_dst = (i_dst + n) % (dst->size + 1);
        nleft -= n;
    }
    /*  Update dst cbuf metadata.
     */
    if (ncopy > 0) {
        dst->i_in = (dst->i_in + ncopy) % (dst->size + 1);
        dst->used += ncopy;
        if (dst->used > dst->size) {
            dst->used = dst->size;
            dst->i_out = (dst->i_in + 1) % (dst->size + 1);
        }
    }
    return(len);
}


static int
cbuf_dropper (cbuf_t cb, int len)
{
/*  Discards exactly [len] bytes of unread data from [cb].
 *  Returns the number of bytes dropped.
 */
    assert(cb != NULL);
    assert(len > 0);
    assert(len <= cb->used);
    assert(cbuf_mutex_is_locked(cb));

    cb->used -= len;
    cb->i_out = (cb->i_out + len) % (cb->size + 1);

    /*  Attempt to shrink cbuf if possible.
     */
    if ((cb->size - cb->used > CBUF_CHUNK) && (cb->size > cb->minsize)) {
        cbuf_shrink(cb);
    }
    /*  Don't call me clumsy, don't call me a fool.
     *  When things fall down on me, I'm following the rule.
     */
    return(len);
}


static int
cbuf_reader (cbuf_t src, int len, cbuf_iof putf, void *dst)
{
/*  Reads up to [len] bytes from [src] into the object pointed at by [dst].
 *    The I/O function [putf] specifies how data is written into [dst].
 *  Returns the number of bytes read, or -1 on error (with errno set).
 *  Note that [dst] is a value-result parameter and will be "moved forward"
 *    by the number of bytes written into it.
 */
    int nleft, n, m = 0;
    int i_src;

    assert(src != NULL);
    assert(len > 0);
    assert(putf != NULL);
    assert(dst != NULL);
    assert(cbuf_mutex_is_locked(src));

    /*  Bound len by the number of bytes available.
     */
    len = MIN(len, src->used);

    /*  Copy data from src cbuf to dst obj.  Do the cbuf hokey-pokey and
     *    wrap-around the buffer at most once.  Break out if putf() returns
     *    either an ERR or a short count.
     */
    nleft = len; /* 0 */
    i_src = src->i_out;
    while (nleft > 0) {
        n = MIN(nleft, (src->size + 1) - i_src);
        m = putf(&src->data[i_src], dst, n);
        if (m > 0) {
            nleft -= m;
            i_src = (i_src + m) % (src->size + 1);
        }
        if (n != m) {
            break;                      /* got ERR or "short" putf() */
        }
    }
    /*  Compute number of bytes written to dst obj.
     */
    n = len - nleft;
    /*
     *  If no data has been written, return the ERR reported by putf().
     */
    if (n == 0) {
        return(m);
    }
    return(n);
}


static int
cbuf_replayer (cbuf_t src, int len, cbuf_iof putf, void *dst)
{
/*  Replays up to [len] bytes from [src] into the object pointed at by [dst].
 *    The I/O function [putf] specifies how data is written into [dst].
 *  Returns the number of bytes replayed, or -1 on error (with errno set).
 *  Note that [dst] is a value-result parameter and will be "moved forward"
 *    by the number of bytes written into it.
 */
    assert(src != NULL);
    assert(len > 0);
    assert(putf != NULL);
    assert(dst != NULL);
    assert(cbuf_mutex_is_locked(src));

    /*  FIXME: NOT IMPLEMENTED.
     */
    errno = ENOSYS;
    return(-1);
}


static int
cbuf_writer (cbuf_t dst, int len, cbuf_iof getf, void *src, int *ndropped)
{
/*  Writes up to [len] bytes from the object pointed at by [src] into [dst].
 *    The I/O function [getf] specifies how data is read from [src].
 *  Returns the number of bytes written, or -1 on error (with errno set).
 *  Sets [ndropped] (if not NULL) to the number of [dst] bytes overwritten.
 *  Note that [src] is a value-result parameter and will be "moved forward"
 *    by the number of bytes read from it.
 */
    int nfree, nleft, n, m;

    assert(dst != NULL);
    assert(len > 0);
    assert(getf != NULL);
    assert(src != NULL);
    assert(cbuf_mutex_is_locked(dst));

    /*  Attempt to grow dst cbuf if necessary.
     */
    nfree = dst->size - dst->used;
    if ((len > nfree) && (dst->size < dst->maxsize)) {
        nfree += cbuf_grow(dst, len - nfree);
    }
    /*  Initialize ndropped in case of early return.
     */
    if (ndropped) {
        *ndropped = 0;
    }
    /*  Compute number of bytes to write to dst cbuf.
     */
    if (dst->overwrite == CBUF_NO_DROP) {
        len = MIN(len, dst->size - dst->used);
        if (len == 0) {
            errno = ENOSPC;
            return(-1);
        }
    }
    else if (dst->overwrite == CBUF_WRAP_ONCE) {
        len = MIN(len, dst->size);
    }
    /*  Copy data from src obj to dst cbuf.  Do the cbuf hokey-pokey and
     *    wrap-around the buffer as needed.  Break out if getf() returns
     *    either an EOF/ERR or a short count.
     */
    nleft = len;
    while (nleft > 0) {
        n = MIN(nleft, (dst->size + 1) - dst->i_in);
        m = getf(&dst->data[dst->i_in], src, n);
        if (m > 0) {
            nleft -= m;
            dst->i_in = (dst->i_in + m) % (dst->size + 1);
        }
        if (n != m) {
            break;                      /* got EOF/ERR or "short" getf() */
        }
    }
    /*  Compute number of bytes written to dst cbuf.
     */
    n = len - nleft;
    /*
     *  If no data has been written, return the EOF/ERR reported by getf().
     */
    if (n == 0) {
        return(m);
    }
    dst->used += n;
    /*
     *  Check to see if any data in the circular-buffer was overwritten.
     */
    if (dst->used > dst->size) {
        dst->used = dst->size;
        dst->i_out = (dst->i_in + 1) % (dst->size + 1);
    }
    if (ndropped) {
        *ndropped = MAX(0, n - nfree);
    }
    return(n);
}


static int
cbuf_grow (cbuf_t cb, int n)
{
/*  Attempts to grow the circular buffer [cb] by at least [n] bytes.
 *  Returns the number of bytes by which the buffer has grown (which may be
 *    less-than, equal-to, or greater-than the number of bytes requested).
 */
    unsigned char *data;                /* tmp ptr to data buffer */
    int size_old;                       /* size of buffer upon func entry */
    int size_meta;                      /* size of sentinel & magic cookies */
    int m;                              /* num bytes to realloc */

    assert(cb != NULL);
    assert(n > 0);
    assert(cbuf_mutex_is_locked(cb));

    if (cb->size == cb->maxsize) {
        return(0);
    }
    size_old = cb->size;
    size_meta = cb->alloc - cb->size;
    assert(size_meta > 0);

    /*  Attempt to grow data buffer by multiples of the chunk-size.
     */
    m = cb->alloc + n;
    m = m + (CBUF_CHUNK - (m % CBUF_CHUNK));
    m = MIN(m, (cb->maxsize + size_meta));
    assert(m > cb->alloc);

    data = cb->data;
#ifndef NDEBUG
    data -= CBUF_MAGIC_LEN;             /* jump back to what malloc returned */
#endif /* !NDEBUG */

    if (!(data = realloc(data, m))) {
        /*
         *  XXX: Set flag to prevent regrowing when out of memory?
         */
        return(0);                      /* unable to grow data buffer */
    }

    cb->data = data;
    cb->alloc = m;
    cb->size = m - size_meta;

#ifndef NDEBUG
    /*  A round cookie with one bite out of it looks like a C.
     *  The underflow cookie will have been copied by realloc() if needed.
     *    But the overflow cookie must be rebaked.
     *  Must use memcpy since overflow cookie may not be word-aligned.
     */
    cb->data += CBUF_MAGIC_LEN;         /* jump forward past underflow magic */
    memcpy(cb->data + cb->size + 1, (void *) &cb->magic, CBUF_MAGIC_LEN);
#endif /* !NDEBUG */

    /*  If unread data wrapped-around the old buffer, move the first chunk
     *    to the new end of the buffer so it wraps-around in the same manner.
     *
     *  XXX: What does this do to the replay buffer?
     *       Replay data should be shifted as well.
     */
    if (cb->i_out > cb->i_in) {
        n = (size_old + 1) - cb->i_out;
        m = (cb->size + 1) - n;
        memmove(cb->data + m, cb->data + cb->i_out, n);
        cb->i_out = m;
    }

    assert(cbuf_is_valid(cb));
    return(cb->size - size_old);
}


static int
cbuf_shrink (cbuf_t cb)
{
/*  XXX: DOCUMENT ME.
 */
    assert(cb != NULL);
    assert(cbuf_mutex_is_locked(cb));
    assert(cbuf_is_valid(cb));

    if (cb->size == cb->minsize) {
        return(0);
    }
    if (cb->size - cb->used <= CBUF_CHUNK) {
        return(0);
    }
    /*  FIXME: NOT IMPLEMENTED.
     */
    assert(cbuf_is_valid(cb));
    return(0);
}


#ifndef NDEBUG
#ifdef WITH_PTHREADS
static int
cbuf_mutex_is_locked (cbuf_t cb)
{
/*  Returns true if the mutex is locked; o/w, returns false.
 */
    int rc;

    assert(cb != NULL);
    rc = pthread_mutex_trylock(&cb->mutex);
    return(rc == EBUSY ? 1 : 0);
}
#endif /* WITH_PTHREADS */
#endif /* !NDEBUG */


#ifndef NDEBUG
static int
cbuf_is_valid (cbuf_t cb)
{
/*  Validates the data structure.  All invariants should be tested here.
 *  Returns true if everything is valid; o/w, aborts due to assertion failure.
 */
    int nfree;

    assert(cb != NULL);
    assert(cbuf_mutex_is_locked(cb));
    assert(cb->data != NULL);
    assert(cb->magic == CBUF_MAGIC);
    /*
     *  Must use memcmp since overflow cookie may not be word-aligned.
     */
    assert(memcmp(cb->data - CBUF_MAGIC_LEN,
        (void *) &cb->magic, CBUF_MAGIC_LEN) == 0);
    assert(memcmp(cb->data + cb->size + 1,
        (void *) &cb->magic, CBUF_MAGIC_LEN) == 0);

    assert(cb->alloc > 0);
    assert(cb->alloc > cb->size);
    assert(cb->size > 0);
    assert(cb->size >= cb->minsize);
    assert(cb->size <= cb->maxsize);
    assert(cb->minsize > 0);
    assert(cb->maxsize > 0);
    assert(cb->used >= 0);
    assert(cb->used <= cb->size);
    assert(cb->overwrite == CBUF_NO_DROP
        || cb->overwrite == CBUF_WRAP_ONCE
        || cb->overwrite == CBUF_WRAP_MANY);
    assert(cb->i_in >= 0);
    assert(cb->i_in <= cb->size);
    assert(cb->i_out >= 0);
    assert(cb->i_out <= cb->size);

    if (cb->i_in == cb->i_out) {
        nfree = cb->size;
    }
    else if (cb->i_in < cb->i_out) {
        nfree = (cb->i_out - cb->i_in) - 1;
    }
    else {
        nfree = ((cb->size + 1) - cb->i_in) + cb->i_out - 1;
    }
    assert(cb->size - cb->used == nfree);

    return(1);
}
#endif /* !NDEBUG */
