/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

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
#define PM_BEACON_ON_RANGED   24
#define PM_BEACON_OFF         25
#define PM_BEACON_OFF_RANGED  26
#define PM_RESOLVE            27
#define NUM_SCRIPTS           28 /* count of scripts above */

#define MAX_MATCH_POS   20

typedef struct {
    InterpState     state;
    char           *str;
    xregex_t        re;
} StateInterp;

typedef struct {
    InterpResult    result;
    char           *str;
    xregex_t        re;
} ResultInterp;

/*
 * A Script is a list of Stmts.
 */
typedef enum {
    STMT_SEND,
    STMT_EXPECT,
    STMT_SETPLUGSTATE,
    STMT_SETRESULT,
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
        struct {                /* SETRESULT (regexs refer to prev expect) */
            int plug_mp;        /* regex subexp match pos of plug name if not */
            int stat_mp;        /* regex subexp match pos of plug status */
            List interps;       /* list of possible interpretations */
        } setresult;
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

typedef struct _device {
    char *name;                 /* name of device */

    char *specname;             /* name of specification, e.g. "icebox3" */

    ConnectState connect_state; /* is device connected/open? */
    bool logged_in;             /* true if login script has run successfully */

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
typedef void (*ActionCB) (int client_id, ActError acterr, const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));
typedef void (*VerbosePrintf) (int client_id, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
typedef void (*DiagPrintf) (int client_id, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

#define MIN_DEV_BUF     1024
#define MAX_DEV_BUF     1024*64

void dev_add(Device * dev);
int dev_enqueue_actions(int com, hostlist_t hl, ActionCB complete_fun,
                        VerbosePrintf vpf_fun, DiagPrintf dpf_fun,
                        int client_id, ArgList arglist);
bool dev_check_actions(int com, hostlist_t hl);

Device *dev_create(const char *name);
void dev_destroy(Device * dev);
Device *dev_findbyname(char *name);
List dev_getdevices(void);

#endif /* PM_DEVICE_PRIVATE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
