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

/* Indices into script array */
#define PM_LOG_IN           0
#define PM_LOG_OUT          1
#define PM_STATUS_PLUGS     2
#define PM_STATUS_PLUGS_ALL 3
#define PM_STATUS_NODES     4
#define PM_STATUS_NODES_ALL 5
#define PM_PING             6
#define PM_POWER_ON         7
#define PM_POWER_ON_ALL     8
#define PM_POWER_OFF        9
#define PM_POWER_OFF_ALL    10
#define PM_POWER_CYCLE      11
#define PM_POWER_CYCLE_ALL  12
#define PM_RESET            13
#define PM_RESET_ALL        14
#define PM_STATUS_TEMP      15
#define PM_STATUS_TEMP_ALL  16
#define PM_STATUS_BEACON    17
#define PM_STATUS_BEACON_ALL 18
#define PM_BEACON_ON        19
#define PM_BEACON_OFF       20
#define PM_RESOLVE          21
/* count of scripts above */
#define NUM_SCRIPTS         22

/*
 * A Script is a list of Stmts.
 */
#define MAX_MATCH_POS   20
typedef enum { STMT_SEND, STMT_EXPECT, STMT_SETSTATUS, STMT_DELAY, 
    STMT_SETPLUGNAME, STMT_FOREACHPLUG, STMT_FOREACHNODE  } StmtType;
typedef struct {
    StmtType type;
    union {
        struct {                /* SEND */
            char *fmt;          /* printf(fmt, ...) style format string */
        } send;
        struct {                /* EXPECT */
            regex_t exp;        /* compiled regex */
        } expect;
        struct {                /* SETSTATUS (regexes refer to prev expect) */
            char *plug_name;    /* plug name if literally specified */
            int plug_mp;        /* regex subexp match pos of plug name if not */
            int stat_mp;        /* regex subexp match pos of plug status */
        } setstatus;
        struct {                /* SETPLUGNAME (regexes refer to prev expect) */
            int plug_mp;        /* regex match position of plug name */
            int node_mp;        /* regex match position of node name */
        } setplugname;
        struct {                /* DELAY */
            struct timeval tv;  /* delay at this point in the script */
        } delay;
        struct {                /* FOREACHPLUG | FOREACHNODE */
            List stmts;         /* list of statements to execute in a loop */
        } foreach;
    } u;
} Stmt;
typedef List Script;

/*
 * Query actions fill ArgList with values and return them to the client.
 */
typedef enum { ST_UNKNOWN, ST_OFF, ST_ON } ArgState;
typedef struct {
    char *node;                 /* node name */
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
typedef enum { DEV_NOT_CONNECTED, DEV_CONNECTING, DEV_CONNECTED } ConnectStat;
typedef enum { TELNET_NONE, TELNET_CMD, TELNET_OPT } TelnetState;
/* bitwise values for dev->script_status */
#define DEV_LOGGED_IN   1
#define DEV_SENDING     2
#define DEV_EXPECTING   4
#define DEV_DELAYING    8

#define DEV_MAGIC       0xbeefb111
typedef struct {
    int magic;
    char *name;                 /* name of device */

    regex_t on_re;              /* regex to match "on" in query */
    regex_t off_re;             /* regex to match "off" in query */

    char *specname;             /* name of specification, e.g. "icebox3" */
    DevType type;               /* type of device e.g. TCP_DEV */
    union {                     /* type-specific device information */
        struct {                /* TCP_DEV | TELNET_DEV */
            char *host;
            char *service;
            TelnetState tstate; /* state of telnet processing */
            unsigned char tcmd; /* buffered command */
        } tcp;
        struct {                /* SERIAL_DEV */
            char *special;
            char *flags;
        } serial;
    } u;

    ConnectStat connect_status;
    int script_status;          /* DEV_* bits reprepsenting script state */

    char *matchstr;             /* cache regex matches for future $N ref */
    regmatch_t pmatch[MAX_MATCH_POS+1];

    int fd;
    List acts;                  /* queue of Actions */

    struct timeval timeout;     /* configurable device timeout */

    cbuf_t to;                  /* buffer -> device */
    cbuf_t from;                /* buffer <- device */

    List plugs;                 /* list of Plugs (node name <-> plug name) */
    Script scripts[NUM_SCRIPTS]; /* array of scripts */

    struct timeval last_retry;  /* time of last reconnect retry */
    int retry_count;            /* number of retries attempted */

    struct timeval last_ping;   /* time of last ping (if any) */
    struct timeval ping_period; /* configurable ping period (0.0 = none) */

    int stat_successful_connects;
    int stat_successful_actions;
} Device;

typedef enum { ACT_ESUCCESS, ACT_EEXPFAIL, ACT_EABORT, ACT_ECONNECTTIMEOUT,
               ACT_ELOGINTIMEOUT } ActError;
typedef void (*ActionCB) (int client_id, ActError acterr, const char *fmt, ...);
typedef void (*VerbosePrintf) (int client_id, const char *fmt, ...);

#define MIN_DEV_BUF     1024
#define MAX_DEV_BUF     1024*1024

void dev_init(void);
void dev_fini(void);
void dev_add(Device * dev);
int dev_enqueue_actions(int com, hostlist_t hl, ActionCB complete_fun, 
        VerbosePrintf vpf_fun, int client_id, ArgList * arglist);
bool dev_check_actions(int com, hostlist_t hl);
void dev_initial_connect(void);

Device *dev_create(const char *name, DevType type);
void dev_destroy(Device * dev);
Device *dev_findbyname(char *name);
List dev_getdevices(void);

Plug *dev_plug_create(const char *name);
int dev_plug_match_plugname(Plug * plug, void *key);
int dev_plug_match_noname(Plug * plug, void *key);
void dev_plug_destroy(Plug * plug);

void dev_pre_select(fd_set * rset, fd_set * wset, int *maxfd);
void dev_post_select(fd_set * rset, fd_set * wset, struct timeval *tv);

ArgList *dev_create_arglist(hostlist_t hl);
ArgList *dev_link_arglist(ArgList * arglist);
void dev_unlink_arglist(ArgList * arglist);

void dev_login(Device *dev);
void dev_disconnect(Device *dev);
bool dev_reconnect(Device *dev);
bool dev_time_to_reconnect(Device * dev, struct timeval *timeout);

#endif                          /* DEVICE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
