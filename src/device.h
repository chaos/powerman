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

#define INTERP_MAGIC 0x13434550
typedef enum { ST_UNKNOWN, ST_OFF, ST_ON } InterpState;
typedef struct {
    int magic;
    InterpState state;
    char *str;
    regex_t *re;
} Interp;

/*
 * A Script is a list of Stmts.
 */
#define MAX_MATCH_POS   20
typedef enum { STMT_SEND, STMT_EXPECT, STMT_SETPLUGSTATE, STMT_DELAY, 
    STMT_SETPLUGNAME, STMT_FOREACHPLUG, STMT_FOREACHNODE, STMT_IFOFF, 
    STMT_IFON  } StmtType;
typedef struct {
    StmtType type;
    union {
        struct {                /* SEND */
            char *fmt;          /* printf(fmt, ...) style format string */
        } send;
        struct {                /* EXPECT */
            regex_t exp;        /* compiled regex */
        } expect;
        struct {                /* SETPLUGSTATE (regexs refer to prev expect) */
            char *plug_name;    /* plug name if literally specified */
            int plug_mp;        /* regex subexp match pos of plug name if not */
            int stat_mp;        /* regex subexp match pos of plug status */
            List interps;       /* list of possible interpretations */
        } setplugstate;
        struct {                /* SETPLUGNAME (regexes refer to prev expect) */
            int plug_mp;        /* regex match position of plug name */
            int node_mp;        /* regex match position of node name */
        } setplugname;
        struct {                /* DELAY */
            struct timeval tv;  /* delay at this point in the script */
        } delay;
        struct {                /* FOREACHPLUG | FOREACHNODE */
            List stmts;         /* list of statements to exec in a loop */
        } foreach;
        struct {                /* IFON | IFOFF */
            List stmts;         /* list of statements to exec conditionally */
        } ifonoff;
    } u;
} Stmt;
typedef List Script;

/*
 * Query actions fill ArgList with values and return them to the client.
 */
typedef struct {
    char *node;                 /* node name */
    char *val;                  /* value as returned by the device */
    InterpState state;          /* interpreted value, if appropriate */
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
typedef enum { DEV_NOT_CONNECTED, DEV_CONNECTING, DEV_CONNECTED } ConnectState;
typedef enum { TELNET_NONE, TELNET_CMD, TELNET_OPT } TelnetState;
typedef enum { TYPE_TCP, TYPE_SERIAL, TYPE_PIPE } DeviceType;

#define DEV_MAGIC       0xbeefb111
typedef struct _device {
    int magic;
    char *name;                 /* name of device */

    char *specname;             /* name of specification, e.g. "icebox3" */

    char *host;                 /* hostname or special file */
    char *port;                 /* port number */
    char *flags;                /* flags (e.g. baud) */

    TelnetState tstate;         /* state of telnet processing */
    unsigned char tcmd;         /* buffered telnet command */

    ConnectState connect_state; /* is device connected/open? */
    bool logged_in;             /* TRUE if login script has run successfully */

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
                                /* connect/disconnect/preprocess methods */
    bool (*connect)(struct _device *dev); 
    bool (*finish_connect)(struct _device *dev);
    void (*preprocess)(struct _device *dev);
    void (*disconnect)(struct _device *dev);
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

Device *dev_create(const char *name);
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

#endif                          /* DEVICE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
