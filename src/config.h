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

/* Review: note: clash with autoconf */
/* Review: consider moving private data structs to .c file */


#ifndef CONFIG_H
#define CONFIG_H

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
#define PM_STATUS_NODES     3
#define PM_PING		    4
#define PM_POWER_ON         5
#define PM_POWER_ON_ALL	    6
#define PM_POWER_OFF        7
#define PM_POWER_OFF_ALL    8
#define PM_POWER_CYCLE      9
#define PM_POWER_CYCLE_ALL  10
#define PM_RESET            11
#define PM_RESET_ALL        12
#define PM_STATUS_TEMP      13
#define PM_STATUS_BEACON    14
#define PM_BEACON_ON        15
#define PM_BEACON_OFF       16
/* count of scripts above */
#define NUM_SCRIPTS	    17

/*
 * Script element (send, expect, or delay).
 */
typedef enum { EL_NONE, EL_SEND, EL_EXPECT, EL_DELAY } Script_El_Type;
typedef struct {
    Script_El_Type type;
    union {
	struct {		/* SEND */
	    char *fmt;		/* printf(fmt, ...) style format string */
	} send;
	struct {		/* EXPECT */
	    regex_t exp;	/* compiled regex */
	    List map;		/* list of Interpretation structures */
	} expect;
	struct {		/* DELAY */
	    struct timeval tv;	/* delay at this point in the script */
	} delay;
    } s_or_e;
} Script_El;

/*
 * Set of scripts - each script is a list of expect/send/delays.
 */
typedef struct {
    List	    scripts[NUM_SCRIPTS]; /* array of lists of Script_El's */
} Protocol;

/*
 * Unprocessed Script_El (used during parsing).
 */
typedef struct {
    Script_El_Type  type;	/* delay/expect/send */
    char *string1;	/* expect string, send fmt */
    struct timeval  tv;		/* delay value */
    List	    map;	/* interpretations */
} Spec_El;

/*
 * Unprocessed Protocol (used during parsing).
 * This data will be copied for each instantiation of a device.
 */
typedef struct {
    char        *name;	/* specification name, e.g. "icebox" */
    Dev_Type    type;	/* device type, e.g. TCP_DEV */
    char        *off;	/* off string, e.g. "OFF" */
    char        *on;		/* on string, e.g. "ON" */
    char        *all;	/* all string, e.g. "*" */
    int		size;	/* number of plugs per device */
    struct timeval  timeout;	/* timeout for this device */
    struct timeval  ping_period; /* ping period for this device 0.0 = none */
    char        **plugname;	/* list of plug names (e.g. "1" thru "10") */
    List	scripts[NUM_SCRIPTS]; /* array of lists of Spec_El's */
} Spec;

/*
 * Interpretation - a Script_El map entry.
 */
typedef struct {
    char	    *plug_name;	/* plug name e.g. "10" */
    int		    match_pos;	/* index into pmatch array after regex call */
    char	    *node;	/* where to update matched values */
} Interpretation;

Protocol *conf_init_client_protocol(void);

Script_El *conf_script_el_create(Script_El_Type type, char *s1, List map, 
		struct timeval tv);
void conf_script_el_destroy(Script_El * script_el);

Spec *conf_spec_create(char *name);
int conf_spec_match(Spec * spec, void *key);
void conf_spec_destroy(Spec * spec);

Spec_El *conf_spec_el_create(Script_El_Type type, char *str1, 
		struct timeval *tv, List map);
void conf_spec_el_destroy(Spec_El * specl);

Interpretation *conf_interp_create(char *name);
int conf_interp_match(Interpretation * interp, void *key);
void conf_interp_destroy(Interpretation * interp);

void conf_init(char *filename);
void conf_fini(void);

void conf_add_spec(Spec *spec);
Spec *conf_find_spec(char *name);

bool conf_addnode(char *node);
bool conf_node_exists(char *node);
hostlist_t conf_getnodes(void);

bool conf_get_use_tcp_wrappers(void);
void conf_set_use_tcp_wrappers(bool val);

int conf_get_listen_port(void);
void conf_set_listen_port(int val);

#endif				/* CONFIG_H */

/*
 * vi:softtabstop=4
 */
