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
#include "string.h"

#define NUM_SCRIPTS	11
#define UPDATE_SECONDS	300
#define NOT_SET		(-1)

/*
 * Script element (send, expect, or delay).
 */
typedef enum { EL_NONE, EL_SEND, EL_EXPECT, EL_DELAY } Script_El_Type;
typedef struct {
    Script_El_Type type;
    union {
	struct {
	    String fmt;		/* printf(fmt, ...) style format string */
	} send;
	struct {
	    regex_t exp;	/* compiled regex */
	    List map;		/* list of Interpretation structures */
	} expect;
	struct {
	    struct timeval tv;	/* delay at this point in the script */
	} delay;
    } s_or_e;
} Script_El;

/*
 * Set of scripts (11) defining a protocol.  
 * Each script is a list of expect/send/delays.
 */
typedef enum { SM_NONE, SM_REGEX, SM_LITERAL } String_Mode;
typedef struct {
    String_Mode	    mode;	/* strings are interpreted as regex/literal */
    List	    scripts[NUM_SCRIPTS]; /* array of lists of Script_El's */
} Protocol;

/*
 * Unprocessed Script_El (used during parsing).
 */
typedef struct {
    Script_El_Type  type;	/* delay/expect/send */
    String	    string1;	/* expect string, send fmt */
    struct timeval  tv;		/* delay value */
    List	    map;	/* interpretations */
} Spec_El;

/*
 * Unprocessed Protocol (used during parsing).
 * This data will be copied for each instantiation of a device.
 */
typedef struct {
    String	    name;	/* specification name, e.g. "icebox" */
    Dev_Type	    type;	/* device type, e.g. TCP_DEV */
    String	    off;	/* off string, e.g. "OFF" */
    String	    on;		/* on string, e.g. "ON" */
    String	    all;	/* all string, e.g. "*" */
    int		    size;	/* number of plugs per device */
    struct timeval  timeout;	/* timeout for this device */
    String	    *plugname;	/* list of plug names (e.g. "1" thru "10") */
    String_Mode	    mode;	/* interp mode, e.g. literal */
    List	    scripts[NUM_SCRIPTS]; /* array of lists of Spec_El's */
} Spec;

/*
 * Node
 */
typedef enum { ST_UNKNOWN, ST_OFF, ST_ON } State_Val; /* plug state */
typedef struct {
    String	    name;	/* hostname */
    State_Val	    p_state;	/* plug state, i.e. hard-power status */
    Device	    *p_dev;	/* device used for plug state */
    int		    p_index;	/* port index into plug state device */
    State_Val	    n_state;	/* node state, i.e. soft-power status */
    Device	    *n_dev;	/* device used for node state */
    int		    n_index;	/* port index into node state device */
    MAGIC;
} Node;

/*
 * Interpretation - a Script_El map entry.
 */
typedef struct {
    String	    plug_name;	/* plug name e.g. "10" */
    int		    match_pos;	/* offset into string where match is */
    char	    *val;	/* pointer based on match_pos */
    Node	    *node;	/* where to update matched values */
} Interpretation;

/*
 * Defines the set of nodes managed by powerman.
 */
typedef struct {
    int		    num;	/* node count */
    List	    nodes;	/* list of Node structures */
} Cluster;


Protocol *conf_init_client_protocol(void);

Script_El *conf_script_el_create(Script_El_Type type, char *s1, List map, 
		struct timeval tv);
void conf_script_el_destroy(Script_El * script_el);

Spec *conf_spec_create(char *name);
int conf_spec_match(Spec * spec, void *key);
void conf_spec_destroy(Spec * spec);

Spec_El *conf_spec_el_create(Script_El_Type type, char *str1, List map);
void conf_spec_el_destroy(Spec_El * specl);

Node *conf_node_create(const char *name);
int conf_node_match(Node * node, void *key);
void conf_node_destroy(Node * node);

Interpretation *conf_interp_create(char *name);
int conf_interp_match(Interpretation * interp, void *key);
void conf_interp_destroy(Interpretation * interp);
void conf_strtotv(struct timeval *tv, char *s);

void conf_init(char *filename);
void conf_fini(void);

void conf_add_spec(Spec *spec);
Spec *conf_find_spec(char *name);

void conf_addnode(Node *node);
List conf_getnodes(void);

void conf_set_select_timeout(struct timeval *tv);
void conf_get_select_timeout(struct timeval *tv);

void conf_set_write_pause(struct timeval *tv);
void conf_get_write_pause(struct timeval *tv);

void conf_set_update_interval(struct timeval *tv);
void conf_get_update_interval(struct timeval *tv);

bool conf_get_use_tcp_wrappers(void);
void conf_set_use_tcp_wrappers(bool val);

int conf_get_listen_port(void);
void conf_set_listen_port(int val);

#endif				/* CONFIG_H */

/*
 * vi:softtabstop=4
 */
