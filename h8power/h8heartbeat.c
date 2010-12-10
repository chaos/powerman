/*****************************************************************************\
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton (uselton2@llnl.gov>
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
/*
 * Manage the cache of SP names.
 * Every 30s, each H8 sends out a heartbeat packet containing the SP name.
 * We cache these and use them to map between RS485 addresss and node names.
 *
 * If h8power was invoked with a hostlist, then we will only cache those names
 * and ignore all others.  If no hostlist was specified, we will cache all 
 * names that show up on the wire.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "serial.h"
#include "hexdump.h"
#include "list.h"
#include "wrappers.h"
#include "hostlist.h"
#include "cbuf.h"
#include "h8proto.h"
#include "h8heartbeat.h"
#include "error.h"

static const uint8_t heartbeat[] =            H8_HEARTBEAT;

static int create_on_refresh = 0;
static time_t start_time;

static List h8hb_cache;
static hostlist_t h8hb_ignore;


/* Create an h8hb struct.
 */
static h8hb_t *_create_h8hb(char *name)
{
    h8hb_t *sp = (h8hb_t *)Malloc(sizeof(h8hb_t));
    memset(sp, 0, sizeof(h8hb_t));

    sp->addr = H8_INVAL_ADDR;
    timerclear(&sp->when);
    timerclear(&sp->last);
    sp->name = Strdup(name);
    sp->logged_dead = 0;

    return sp;
}

/* Destroy an h8hb struct.
 */
static void _destroy_h8hb(h8hb_t *sp)
{
    if (sp) {
        if (sp->name)
            Free(sp->name);
        Free(sp);
    }
}

/* Create an h8hb iterator.
 */
h8hb_iterator_t h8hb_iterator_create(void)
{
    return list_iterator_create(h8hb_cache);
}

/* Destroy an h8hb iterator.
 */
void h8hb_iterator_destroy(h8hb_iterator_t itr)
{
    list_iterator_destroy(itr);
}

/* Return next cache item for an h8hb iterator.
 */
h8hb_t *h8hb_next(h8hb_iterator_t itr)
{
    return list_next(itr);
}

static int _find_by_addr(h8hb_t *sp, uint8_t *key)
{
    return (sp->addr == *key);
}

static int _find_by_name(h8hb_t *sp, char *key)
{
    return (strcmp(sp->name, key) == 0);
}

/* Refresh cache entry - called from heartbeat handler.
 */
static void _refresh(uint8_t addr, char *name)
{
    h8hb_t *sp;

    if (hostlist_find(h8hb_ignore, name) == -1) { /* not in ignore list */
        sp = list_find_first(h8hb_cache, (ListFindF)_find_by_name, name);
        if (!sp && create_on_refresh) {
            sp = _create_h8hb(name);
            list_append(h8hb_cache, sp);
        }
        if (sp) {
            if (!h8hb_expired(sp) && sp->addr != addr) {
                err("%s: address changed from 0x%-2.2x to 0x%-2.2x!", 
                        name, sp->addr, addr);
            }
            sp->addr = addr;
            sp->last = sp->when;
            Gettimeofday(&sp->when, NULL);
        } else
            err("unexpected heartbeat from: %s (0x%-2.2x)", name, addr);
    }
}

/* return time to live in seconds for a cache entry */
static int _ttl(h8hb_t *sp)
{
    struct timeval hb_age, now;

    Gettimeofday(&now, NULL);
    timersub(&now , &sp->when, &hb_age);     /* hb_age = now - sp->when */
    return (H8_SPNAME_CACHE_TTL - hb_age.tv_sec);
}

/* Return 1 if heartbeat cache entry is expired, 0 if not.
 * Log the lack of heartbeat for sys admin RS-485 chain debugging purposes.
 */
int h8hb_expired(h8hb_t *sp)
{
    int exp = _ttl(sp) <= 0;
#if 0
    if (exp && (Time(0) - start_time > H8_HEARTBEAT_PERIOD*2))
        err("no heartbeat from %s", sp->name);
#endif
    return exp;
} 

/* Display the heartbeat cache.
 */
void h8hb_display_cache(cbuf_t ttyout)
{
    h8hb_t *sp;
    struct timeval hb_age, hb_period, now;
    ListIterator itr;

    Gettimeofday(&now, NULL);
    itr = list_iterator_create(h8hb_cache);
    while ((sp = list_next(itr)) != NULL) {
        timersub(&now , &sp->when, &hb_age);     /* hb_age = now - sp->when */
        timersub(&sp->when, &sp->last, &hb_period);
        if (!h8hb_expired(sp) > 0) {
            cbuf_printf(ttyout, "%s: up (0x%-2.2x) last heartbeat %ds ago", 
                    sp->name, sp->addr, hb_age.tv_sec);
            if (timerisset(&sp->last))
                cbuf_printf(ttyout, "(T=%-4.4ld.%2.2lds)\n", 
                        hb_period.tv_sec, hb_period.tv_usec/10000L);
            else
                cbuf_printf(ttyout, "\n");
        } else if (!timerisset(&sp->when))
            cbuf_printf(ttyout, "%s: down forever\n", sp->name);
        else 
            cbuf_printf(ttyout, "%s: down last heartbeat %ds ago\n", sp->name,
                hb_age.tv_sec);
    }
    list_iterator_destroy(itr);
}

/* Find a heartbeat entry by its address.
 */
h8hb_t *h8hb_lookup_byaddr(uint8_t addr)
{
    return list_find_first(h8hb_cache, (ListFindF)_find_by_addr, &addr);
}

/* Find a heartbeat entry by its name.
 */
h8hb_t *h8hb_lookup_byname(char *name)
{
    return list_find_first(h8hb_cache, (ListFindF)_find_by_name, name);
}

static void _usleep(int usec)
{
    struct timeval tv;
    Pollfd_t pfd;

    pfd = PollfdCreate();

    tv.tv_usec = usec % 1000000;
    tv.tv_sec  = usec / 1000000;

    if (Poll(pfd, &tv) < 0)
        err_exit("_usleep: poll error");

    PollfdDestroy(pfd);
}

/* XXX This is sort of evil, but since the wire is quiet when we're not 
 * talking except for heartbeats, and heartbeats occur at regular 32s
 * intervals, we can use the time of the last heartbeat to predict when the
 * wire will be busy and use that information to avoid collisions.  Why not?
 */
#define H8_MAX_TRANSACTION_TIME 200 /* ms */
#define H8_WILL_SLEEP   H8_MAX_TRANSACTION_TIME * 1
#define H8_SLEEP_TIME   H8_MAX_TRANSACTION_TIME * 3
void h8hb_sleep_through_heartbeats(void)
{
    ListIterator itr;
    h8hb_t *sp;
    struct timeval now, next, until;

    itr = list_iterator_create(h8hb_cache);
    while ((sp = list_next(itr)) != NULL) {
        Gettimeofday(&now, 0);
        if (!h8hb_expired(sp) > 0) {
            next = sp->when;
            do {
                next.tv_sec += H8_HEARTBEAT_PERIOD;
            } while (timercmp(&now, &next, >));     /* while now > next */
            timersub(&next, &now, &until);          /* until = next - now */
            if (until.tv_sec == 0 
                    && until.tv_usec <= H8_WILL_SLEEP * 1000L) {
                _usleep(H8_SLEEP_TIME * 1000L);
            }
        }
    }
    list_iterator_destroy(itr);
}

/* Return a pointer to null-terminated SP name in heartbeat packet,
 * or NULL if it is not null-terminated.
 */
static char *_heartbeat_spname(h8pkt_t *pkt)
{
    int i;

    assert(pkt->magic == H8PKT_MAGIC);
    for (i = H8_SPNAME_OFFSET; i < *pkt->len; i++) {
        if (pkt->data[i] == '\0')
            break;
    }
    return (i >= *pkt->len ? NULL : &pkt->data[H8_SPNAME_OFFSET]);
}

int h8hb_process_packet(h8pkt_t *pkt)
{
    char *spname;
    int consumed = 0;

    if (H8_MATCH_DATA(pkt, heartbeat)) {
        if ((spname = _heartbeat_spname(pkt)))
            _refresh(*pkt->src, spname);
        consumed = 1;
    }
    return consumed;
}

#define HEARTBEAT_LATE_CHECK 60L /* seconds */
void h8hb_process_late_heartbeats(struct timeval *tmout)
{
    static struct timeval interval = { HEARTBEAT_LATE_CHECK, 0L };
    ListIterator itr;
    h8hb_t *sp;

    if (Time(0) - start_time > H8_SPNAME_CACHE_TTL) { 
        itr = list_iterator_create(h8hb_cache);
        while ((sp = list_next(itr))) {
            if (_ttl(sp) <= 0 && !sp->logged_dead) {
                err("no heartbeat from %s", sp->name);
                sp->logged_dead = 1;
            } else if (_ttl(sp) > 0 && sp->logged_dead) {
                err("heartbeat restored from %s", sp->name);
                sp->logged_dead = 0;
            } 
        }
        list_iterator_destroy(itr);
    }

    if (!timerisset(tmout) || timercmp(tmout, &interval, >))
        *tmout = interval;
}

/* Initialize the heartbeat cache with a hostlist from the command line,
 * or if the hostlist is empty, set the 'create_on_refresh' flag.
 */
void h8hb_init(char *spnames, char *ignore)
{
    h8hb_cache = list_create((ListDelF)_destroy_h8hb);

    h8hb_ignore = hostlist_create(NULL);
    if (h8hb_ignore == NULL)
        err_exit("h8hb_init: hostlist_create failed");

    if (spnames) {
        hostlist_t hl = hostlist_create(spnames);
        hostlist_iterator_t itr; 
        char *name;

        if (hl == NULL)
            err_exit("h8hb_init: hostlist_create failed");
        itr = hostlist_iterator_create(hl);
        while ((name = hostlist_next(itr)) != NULL)
            list_append(h8hb_cache, _create_h8hb(name));
        hostlist_iterator_destroy(itr);
    } else {
        create_on_refresh = 1;
    }

    if (ignore) {
        if (hostlist_push(h8hb_ignore, ignore) == 0)
            err_exit("h8hb_init: hostlist_push failed on ignore list");
    }

    Time(&start_time);
}

/* Destroy the heartbeat cache.
 */
void h8hb_fini(void)
{
    list_destroy(h8hb_cache);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
