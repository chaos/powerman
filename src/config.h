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
 * Devices:
 * Older iceboxes and WTI units communicated via a serial port (TTY).
 * Newer iceboxes use a variety of RAW TCP communication.  Baytechs and
 * newer WTI units use a telnet listener.  There is some speculation
 * that we may see SNMP based units eventually.  The initial vicebox
 * design will only handle RAW TCP.  I may add TTY if it ends up working
 * in time.  
 *
 *   N.B.  Though this enumeration properly belongs in the Devices
 * section there is some dependency on it that I din't diagnose
 * that make the compiler happier if this is here.  Hrmph.
 */

/*
 *   Script_El structures come in three varieties.  Script elements
 * may be "SEND" objects with a single "printf(fmt, ...)" style fmt
 * string, "EXPECT" objects with two compiled RegEx objects - one for 
 * recognizing when a complete input is in the buffer, the other for 
 * matching in in conjunction with Interpretation structures - 
 * and "DELAY" objects that have a single struct timeval and cause 
 * that much time to be wasted at that point in the script.
 */
typedef enum { SND_EXP_UNKNOWN, SEND, EXPECT, DELAY } Script_El_T;

typedef struct {
    String fmt;
} Send_T;

/* Review: noodle on necessity of completion regex */
/* the map is a list of Interpretation structures */
typedef struct {
    regex_t completion;
    regex_t exp;
    List map;
} Expect_T;

typedef struct {
    struct timeval tv;
} Delay_T;

typedef union {
    Send_T send;
    Expect_T expect;
    Delay_T delay;
} S_E_U;

typedef struct {
    Script_El_T type;
    S_E_U s_or_e;		/* send => fmt */
    /* expect => completion, exp */
    /* delay => tv */
} Script_El;

/*
 * The protocol passes around a string as its target of action. 
 * Some devices only know "port#" or "all" (all actual devices) 
 * as a target string, others (powermand device protocol) 
 * understand and pass around regexes.  Some devices do have
 * syntax for specifying multiple target ports, but it varies
 * from one device to the next, and specifying it in a general
 * way would be burdensome.  
 */
typedef enum { NO_MODE, REGEX, LITERAL } String_Mode;

/*
 *   Each Device gets a Protocol structure and there is on 
 * degenerate one for the client protocol.  In addition to
 * having num_scripts (always == 11) scripts the mode lets
 * the interface between powermand and powerman use RegEx
 * strings instead of lieteral strings.  The eleven lists
 * in "scripts" are all of Script_El structures.
 */
/* Review: convert array of lists to 11 individual items */
typedef struct {
    int num_scripts;
    String_Mode mode;
    List *scripts;		/* description of the send/expect   */
} Protocol;

/*
 *  A Spec_El is the embodiment of what will become a Send, an 
 * Expect, or a Delay in a Protocol.  The Expect's regex
 * is kept uncompiled here, or the Send's string is in the string1 field.
 */
/* Review: should be private */
typedef struct spec_element_struct {
    Script_El_T type;
    String string1;
    struct timeval tv;
    List map;
} Spec_El;

/*
 *  A structure of type Spec holds the abstract embodiment of
 * what will end up in each of (potentially) many Protocol structs.
 */
/* Review: should be private */
typedef struct {
    String name;
    Dev_Type type;
    String off;
    String on;
    String all;
    int size;
    struct timeval timeout;
    int num_scripts;
    String *plugname;
    String_Mode mode;
    List *scripts;		/* An array of pointers to lists */
} Spec;

/* 
 *   Each node and each plug in a cluster is represented by a state
 * intialized to ST_UNKNOWN, but usually ON or OFF.  For each such
 * value there is also a pointer to the device it is connected to
 * and the index in the device.
 */
typedef enum { ST_UNKNOWN, ST_OFF, ST_ON } State_Val;


typedef struct {
    String name;		/* This is how the node is known to the cluster  */
    State_Val p_state;		/* p_state is plug state, i.e. hard-power status */
    Device *p_dev;		/* It is possible for there to be two different  */
    int p_index;		/*   devices, on for har- and one for soft-power */
    State_Val n_state;		/* n_state is node state, i.e. soft-power status */
    Device *n_dev;		/* The (Device, index) pair tell where the node  */
    int n_index;		/* is managed.                                   */
     MAGIC;
} Node;

/*
 *   An EXPECT script element employs a list of these to pull apart
 * the string it has matched.  The regexec() function sets up an 
 * array of indexes to any substrings matched in the RegEx.  Each 
 * Interpretation associates one such substring index with a particular 
 * plug of the device in question.  "val" is set to point to the 
 * place in the matched string indicated by the "match_pos" index.
 * "node" is initialized at the time the config file is read to point
 * to the corresponding Node structure so the semantic value of
 * "val" may be updated.  Perfectly clear right?
 */
typedef struct {
    String plug_name;
    int match_pos;
    char *val;
    Node *node;
} Interpretation;


typedef struct {
    int num;			/* node count */
    List nodes;			/* list of Node structures */
    struct timeval time_stamp;	/* last update */
    struct timeval update_interval;	/* how long before next update */
} Cluster;


Protocol *conf_init_client_protocol(void);

Script_El *conf_script_el_create(Script_El_T type, char *s1, List map, 
		struct timeval tv);
void conf_script_el_destroy(Script_El * script_el);

Spec *conf_spec_create(char *name);
int conf_spec_match(Spec * spec, void *key);
void conf_spec_destroy(Spec * spec);

Spec_El *conf_spec_el_create(Script_El_T type, char *str1, List map);
void conf_spec_el_destroy(Spec_El * specl);

Cluster *conf_cluster_create(void);
void conf_cluster_destroy(Cluster * cluster);

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

void conf_set_select_timeout(struct timeval *tv);
void conf_get_select_timeout(struct timeval *tv);

void conf_set_write_pause(struct timeval *tv);
void conf_get_write_pause(struct timeval *tv);

bool conf_get_use_tcp_wrappers(void);
void conf_set_use_tcp_wrappers(bool val);

unsigned short conf_get_listen_port(void);
void conf_set_listen_port(unsigned short val);


#endif				/* CONFIG_H */
