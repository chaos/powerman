/*****************************************************************************\
 *  $Id$
 *****************************************************************************
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

#ifndef DEVICE_H
#define DEVICE_H

#include <regex.h>
#include "cbuf.h"
#include "hostlist.h"

/*
 * Query actions fill ArgList with values and return them to the client.
 */
typedef enum { ST_UNKNOWN, ST_OFF, ST_ON } ArgState;
typedef struct {
    char *node;                 /* key */
    char *val;                  /* value as returned by the device */
    ArgState state;             /* interpreted value, if appropriate */
} Arg;
typedef struct {
    List argv;                  /* list of Arg structures */
    int refcount;               /* free when refcount == 0 */
} ArgList;

/*
 * Plug maps plug name/number to a node name.  Each device maintains a list.
 */
typedef struct {
    char *name;                 /* how the plug is known to the device */
    char *node;                 /* node name */
} Plug;

/*
 * Device
 */
typedef enum { DEV_NOT_CONNECTED, DEV_CONNECTING,
        DEV_CONNECTED } ConnectStat;
/* bitwise values for dev->script_status */
#define DEV_LOGGED_IN   1
#define DEV_SENDING     2
#define DEV_EXPECTING   4
#define DEV_DELAYING    8
typedef struct {
    char *name;                 /* name of device */

    char *all;                  /* string to select all plugs */
    regex_t on_re;              /* regex to match "on" in query */
    regex_t off_re;             /* regex to match "off" in query */

    char *specname;             /* name of specification, e.g. "icebox3" */
    DevType type;               /* type of device e.g. TCP_DEV */
    union {                     /* type-specific device information */
        struct {
            char *host;
            char *service;
        } tcpd;
    } devu;

    ConnectStat connect_status;
    int script_status;          /* DEV_* bits reprepsenting script state */

    int fd;
    List acts;                  /* queue of Actions */

    struct timeval timeout;     /* configurable device timeout */

    cbuf_t to;                  /* buffer -> device */
    cbuf_t from;                /* buffer <- device */

    List plugs;                 /* list of Plugs */
    Script scripts[NUM_SCRIPTS]; /* array of scripts */

    struct timeval last_retry;  /* time of last reconnect retry */
    int retry_count;            /* number of retries attempted */

    struct timeval last_ping;   /* time of last ping (if any) */
    struct timeval ping_period; /* configurable ping period (0.0 = none) */

    int stat_successful_connects;
    int stat_successful_actions;

     MAGIC;
} Device;

typedef void (*ActionCB) (int client_id, char *errfmt, char *errarg);

void dev_init(void);
void dev_fini(void);
void dev_add(Device * dev);
int dev_enqueue_actions(int com, hostlist_t hl, ActionCB fun,
                        int client_id, ArgList * arglist);
bool dev_check_actions(int com, hostlist_t hl);
void dev_initial_connect(void);

Device *dev_create(const char *name);
void dev_destroy(Device * dev);
Device *dev_findbyname(char *name);
List dev_getdevices(void);

Plug *dev_plug_create(const char *name);
int dev_plug_match(Plug * plug, void *key);
void dev_plug_destroy(Plug * plug);

void dev_pre_select(fd_set * rset, fd_set * wset, int *maxfd);
void dev_post_select(fd_set * rset, fd_set * wset, struct timeval *tv);

ArgList *dev_create_arglist(hostlist_t hl);
ArgList *dev_link_arglist(ArgList * arglist);
void dev_unlink_arglist(ArgList * arglist);

#endif                          /* DEVICE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
