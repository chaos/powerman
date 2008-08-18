/*****************************************************************************\
 *  $Id: device.h 912 2008-05-31 01:28:46Z garlick $
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

#ifndef PM_DEVICE_PRIVATE_H
#define PM_DEVICE_PRIVATE_H

#define NO_FD (-1)

/* Indices into script array */
#define PM_LOG_IN             0
#define PM_LOG_OUT            1
#define PM_STATUS_PLUGS       2
#define PM_STATUS_PLUGS_ALL   3
/*4*/
/*5*/
#define PM_PING               6
#define PM_POWER_ON           7
#define PM_POWER_ON_RANGED    8
#define PM_POWER_ON_ALL       9
#define PM_POWER_OFF          10
#define PM_POWER_OFF_RANGED   11
#define PM_POWER_OFF_ALL      12
#define PM_POWER_CYCLE        13
#define PM_POWER_CYCLE_RANGED 14
#define PM_POWER_CYCLE_ALL    15
#define PM_RESET              16
#define PM_RESET_RANGED       17
#define PM_RESET_ALL          18
#define PM_STATUS_TEMP        19
#define PM_STATUS_TEMP_ALL    20
#define PM_STATUS_BEACON      21
#define PM_STATUS_BEACON_ALL  22
#define PM_BEACON_ON          23
#define PM_BEACON_OFF         24
#define PM_RESOLVE            25
#define NUM_SCRIPTS           26 /* count of scripts above */

#define MAX_MATCH_POS   20 

#define INTERP_MAGIC 0x13434550
typedef struct {
    int             magic;
    InterpState     state;
    char           *str;
    xregex_t        re;
} Interp;

/*
 * A Script is a list of Stmts.
 */
typedef enum { 
    STMT_SEND, 
    STMT_EXPECT, 
    STMT_SETPLUGSTATE, 
    STMT_DELAY, 
    STMT_FOREACHPLUG, 
    STMT_FOREACHNODE, 
    STMT_IFOFF, 
    STMT_IFON,
} StmtType;

typedef struct {
    StmtType type;
    union {
        struct {                /* SEND */
            char *fmt;          /* printf(fmt, ...) style format string */
        } send;
        struct {                /* EXPECT */
            xregex_t exp;        /* compiled regex */
        } expect;
        struct {                /* SETPLUGSTATE (regexs refer to prev expect) */
            char *plug_name;    /* plug name if literally specified */
            int plug_mp;        /* regex subexp match pos of plug name if not */
            int stat_mp;        /* regex subexp match pos of plug status */
            List interps;       /* list of possible interpretations */
        } setplugstate;
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
 * Device
 */
typedef enum { DEV_NOT_CONNECTED, DEV_CONNECTING, DEV_CONNECTED } ConnectState;

#define DEV_MAGIC       0xbeefb111
typedef struct _device {
    int magic;
    char *name;                 /* name of device */

    char *specname;             /* name of specification, e.g. "icebox3" */

    ConnectState connect_state; /* is device connected/open? */
    bool logged_in;             /* TRUE if login script has run successfully */

    xregex_match_t xmatch;      /* cache regex matches for future $N ref */

    int fd;                     /* socket, serial device, or pty */

    List acts;                  /* queue of Actions */

    struct timeval timeout;     /* configurable device timeout */

    cbuf_t to;                  /* buffer -> device */
    cbuf_t from;                /* buffer <- device */

    PlugList plugs;             /* list of Plugs (node name <-> plug name) */
    Script scripts[NUM_SCRIPTS]; /* array of scripts */

    struct timeval last_retry;  /* time of last reconnect retry */
    int retry_count;            /* number of retries attempted */

    struct timeval last_ping;   /* time of last ping (if any) */
    struct timeval ping_period; /* configurable ping period (0.0 = none) */

    int stat_successful_connects;
    int stat_successful_actions;
                                /* network (e.g. tcp/serial)-specific methods */
    bool (*connect)(struct _device *dev); 
    bool (*finish_connect)(struct _device *dev);
    void (*preprocess)(struct _device *dev);
    void (*disconnect)(struct _device *dev);
    void (*destroy)(void *data);

    void *data;
} Device;

typedef enum { ACT_ESUCCESS, ACT_EEXPFAIL, ACT_EABORT, ACT_ECONNECTTIMEOUT,
               ACT_ELOGINTIMEOUT } ActError;
typedef void (*ActionCB) (int client_id, ActError acterr, const char *fmt, ...);
typedef void (*VerbosePrintf) (int client_id, const char *fmt, ...);

#define MIN_DEV_BUF     1024
#define MAX_DEV_BUF     1024*64

void dev_add(Device * dev);
int dev_enqueue_actions(int com, hostlist_t hl, ActionCB complete_fun, 
        VerbosePrintf vpf_fun, int client_id, ArgList arglist);
bool dev_check_actions(int com, hostlist_t hl);

Device *dev_create(const char *name);
void dev_destroy(Device * dev);
Device *dev_findbyname(char *name);
List dev_getdevices(void);

#endif /* PM_DEVICE_PRIVATE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
