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

/* Review: consider moving private data structs to .c file */


#ifndef PARSE_UTIL_H
#define PARSE_UTIL_H

#include <regex.h>
#include <sys/time.h>
#include <sys/types.h>

#include "list.h"
#include "hostlist.h"
#include "string.h"

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
    STMT_SETPLUGNAME  } StmtType;
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
    } u;
} Stmt;
typedef List Script;

/*
 * A PreScript is a list of PreStmts.
 */
typedef struct {
    StmtType type;              /* delay/expect/send */
    char *str;                  /* expect string, send fmt, setstatus plug */
    struct timeval tv;          /* delay value */
    int mp1;                    /* setstatus plug, setplugname plug match pos */
    int mp2;                    /* settatus stat, setplugname node match pos */
} PreStmt;
typedef List PreScript;

/*
 * Unprocessed Protocol (used during parsing).
 * This data will be copied for each instantiation of a device.
 */
typedef struct {
    char *name;                 /* specification name, e.g. "icebox" */
    DevType type;               /* device type, e.g. TCP_DEV */
    char *off;                  /* off string, e.g. "OFF" */
    char *on;                   /* on string, e.g. "ON" */
    int size;                   /* number of plugs per device */
    struct timeval timeout;     /* timeout for this device */
    struct timeval ping_period; /* ping period for this device 0.0 = none */
    char **plugname;            /* list of plug names (e.g. "1" thru "10") */
    PreScript prescripts[NUM_SCRIPTS];  /* array of PreScripts */
} Spec;                                 /*   script may be NULL if undefined */

Stmt *conf_stmt_create(StmtType type, char *str, struct timeval tv, 
        int plug_mp, int stat_or_node_mp);
void conf_stmt_destroy(Stmt * stmt);

Spec *conf_spec_create(char *name);
int conf_spec_match(Spec * spec, void *key);
void conf_spec_destroy(Spec * spec);

PreStmt *conf_prestmt_create(StmtType type, char *str, struct timeval tv,
                int mp1, int mp2);
void conf_prestmt_destroy(PreStmt * specl);

void conf_init(char *filename);
void conf_fini(void);

void conf_add_spec(Spec * spec);
Spec *conf_find_spec(char *name);

bool conf_addnode(char *node);
bool conf_node_exists(char *node);
hostlist_t conf_getnodes(void);

bool conf_get_use_tcp_wrappers(void);
void conf_set_use_tcp_wrappers(bool val);

int conf_get_listen_port(void);
void conf_set_listen_port(int val);

void conf_exp_aliases(hostlist_t hl);
bool conf_add_alias(char *name, char *hosts);

#endif                          /* PARSE_UTIL_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
