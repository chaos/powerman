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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "powerman.h"
#include "error.h"
#include "list.h"
#include "config.h"
#include "device.h"
#include "action.h"
#include "wrappers.h"
#include "config.h"
#include "client.h"

static int _spec_match(Spec *spec, void *key);
static void _spec_destroy(Spec *spec);

static Cluster *_cluster_create(void);
static void _cluster_destroy(Cluster * cluster);

/* 
 * Device specifications only exist during parsing.
 * After that their contents are copied for each instantiation of the device and
 * the original specification is not used.
 */
static List		tmp_specs = NULL;                

/*
 * Some configurable values - accessor functions defined below.
 */
static bool		conf_use_tcp_wrap = FALSE;
static int		conf_listen_port = NO_PORT;
static Cluster	        *conf_cluster = NULL;

/* 
 * initialize module
 * parse the named config file - on error, exit with a message to stderr 
 */
void
conf_init(char *filename)
{
    struct stat stbuf;
    int parse_config_file(char *filename);

    /* initialize cluster */
    conf_cluster = _cluster_create();

    tmp_specs = list_create((ListDelF) _spec_destroy);

    /* validate config file */
    if (stat(filename, &stbuf) < 0)
	err_exit(TRUE, "%s", filename);
    if ((stbuf.st_mode & S_IFMT) != S_IFREG)
	err_exit(FALSE, "%s is not a regular file\n", filename);

    /* 
     * Call yacc parser against config file.  The parser calls support 
     * functions below and builds 'dev_devices' (devices.c),
     * 'conf_cluster', 'tmp_specs' (config.c), and various other 
     * conf_* attributes (config.c).
     */
    parse_config_file(filename);

    list_destroy(tmp_specs);
    tmp_specs = NULL;
}

/* finalize module */
void
conf_fini(void)
{
    _cluster_destroy(conf_cluster);
}

/*******************************************************************
 *                                                                 *
 * Script_El                                                       *
 *   A Script_El structure is an element in a script.  The script  *
 * is a list of these elemments that implements one of the actions *
 * that clients may request of devices.                            *
 *                                                                 *
 *******************************************************************/

Script_El *conf_script_el_create(Script_El_Type type, char *s1,
		List map, struct timeval tv)
{
    Script_El *script_el;
    reg_syntax_t cflags = REG_EXTENDED;

    script_el = (Script_El *) Malloc(sizeof(Script_El));
    script_el->type = type;
    switch (type) {
    case EL_SEND:
	script_el->s_or_e.send.fmt = Strdup(s1);
	break;
    case EL_EXPECT:
	Regcomp(&(script_el->s_or_e.expect.exp), s1, cflags);
	script_el->s_or_e.expect.map = map;
	break;
    case EL_DELAY:
	script_el->s_or_e.delay.tv = tv;
	break;
    default:
    }
    return script_el;
}

void conf_script_el_destroy(Script_El * script_el)
{
    assert(script_el != NULL);

    switch (script_el->type) {
    case EL_SEND:
	Free(script_el->s_or_e.send.fmt);
	break;
    case EL_EXPECT:
    if (script_el->s_or_e.expect.map != NULL)
        list_destroy(script_el->s_or_e.expect.map);
	break;
    case EL_DELAY:
    default:
    }
    Free(script_el);
}

/*******************************************************************
 *                                                                 *
 * Spec                                                            *
 *    A Spec struct holds all the information about a device type  *
 * while the config file is being parsed.  Device lines refer to   *
 * entries in the Spec list to get their detailed construction     *
 * and configuration information.  The Spec is not used after the  *
 * config file is closed.                                          *
 *                                                                 *
 *******************************************************************/

Spec *conf_spec_create(char *name)
{
    Spec *spec;
    int i;

    spec = (Spec *) Malloc(sizeof(Spec));
    spec->name = Strdup(name);
    spec->type = NO_DEV;
    spec->size = 0;
    spec->timeout.tv_sec = 0;
    spec->timeout.tv_usec = 0;
    for (i = 0; i < NUM_SCRIPTS; i++)
	spec->scripts[i] = NULL;
    return spec;
}

/* list 'match' function for conf_find_spec() */
static int _spec_match(Spec * spec, void *key)
{
    return (strcmp(spec->name, (char *)key) == 0);
}

/* list destructor for tmp_specs */
static void _spec_destroy(Spec * spec)
{
    int i;

    Free(spec->name);
    Free(spec->off);
    Free(spec->on);
    Free(spec->all);

    for (i = 0; i < spec->size; i++)
	Free(spec->plugname[i]);
    Free(spec->plugname);
    for (i = 0; i < NUM_SCRIPTS; i++)
	list_destroy(spec->scripts[i]);
    Free(spec);
}

/* add a Spec to the internal list */
void
conf_add_spec(Spec *spec)
{
    assert(tmp_specs != NULL);
    list_append(tmp_specs, spec);
}

/* find and return a Spec from the internal list */
Spec *
conf_find_spec(char *name)
{   
    return list_find_first(tmp_specs, (ListFindF) _spec_match, name);
}

/*******************************************************************
 *                                                                 *
 * Spec_El                                                         *
 *   A Spec_El structure is an object associated with a Spec       *
 * that holds information for a Script_El.  The Script_El itself   *
 * cannot be constructed unitl it is added to the device's         *
 * protocol.  All the infromation the Script_El needs at that time *
 * is present in the Spec_El.                                      *
 *                                                                 *
 *******************************************************************/

Spec_El *conf_spec_el_create(Script_El_Type type, char *str1, 
		struct timeval *tv, List map)
{
    Spec_El *specl;

    specl = (Spec_El *) Malloc(sizeof(Spec_El));
    memset(specl, 0, sizeof(Spec_El));
    specl->type = type;
    switch (type) {
    case EL_SEND:
    case EL_EXPECT:
	assert(str1 != NULL && tv == NULL);
	specl->string1 = Strdup(str1);
	break;
    case EL_DELAY:
	assert(str1 == NULL && tv != NULL);
	specl->tv = *tv;
	break;
    default:
	assert(FALSE);
    }
    specl->map = map;
    return specl;
}

void conf_spec_el_destroy(Spec_El * specl)
{
    if (specl->string1 != NULL)
	Free(specl->string1);
    specl->string1 = NULL;
    if (specl->map != NULL)
	list_destroy(specl->map);
    specl->map = NULL;
    Free(specl);
}
/*******************************************************************
 *                                                                 *
 * Cluster                                                         *
 *   A Cluster structure holds the list of Nodes in the cluster    *
 * along with how many there are and when their state was last     *
 * updated.                                                        *
 *                                                                 *
 *******************************************************************/

static Cluster *_cluster_create(void)
{
    Cluster *cluster;

    cluster = (Cluster *) Malloc(sizeof(Cluster));
    cluster->num = 0;
    cluster->nodes = list_create((ListDelF) conf_node_destroy);
    return cluster;
}


static void _cluster_destroy(Cluster * cluster)
{
    list_destroy(cluster->nodes);
    Free(cluster);
}

/*******************************************************************
 *                                                                 *
 * Node                                                            *
 *   A Node structure holds the information for a single computer  *
 * in the cluster.  It has a name and hard- and soft-power state   *
 * as well as pointers indication with Device struct(s) are        *
 * responsible for managing this Node.                             *
 *                                                                 *
 *******************************************************************/

Node *conf_node_create(const char *name)
{
    Node *node;


    node = (Node *) Malloc(sizeof(Node));
    INIT_MAGIC(node);
    node->name = Strdup(name);
    node->p_state = ST_UNKNOWN;
    node->n_state = ST_UNKNOWN;
    node->dev = NULL;
    return node;
}

int conf_node_match(Node * node, void *key)
{
    return (strcmp(node->name, (char *) key) == 0);
}

void conf_node_destroy(Node * node)
{
    Free(node->name);
    Free(node);
}

void conf_addnode(Node *node)
{
    conf_cluster->num++;
    list_append(conf_cluster->nodes, node);
}

List conf_getnodes(void)
{
    return conf_cluster->nodes;
}

/*******************************************************************
 *                                                                 *
 * Interpretation                                                  *
 *   An Interpretation structure is the means by which an EXPECT   *
 * script element identifies the pieces of the RegEx matched in an *
 * may be interpreted to give state information.                   *
 *                                                                 *
 *******************************************************************/

Interpretation *conf_interp_create(char *name)
{
    Interpretation *interp;

    interp = (Interpretation *) Malloc(sizeof(Interpretation));
    interp->plug_name = Strdup(name);
    interp->match_pos = NOT_SET;
    interp->val = NULL;
    interp->node = NULL;
    return interp;
}

int conf_interp_match(Interpretation * interp, void *key)
{
    return (strcmp(interp->plug_name, (char *) key) == 0);
}

void conf_interp_destroy(Interpretation * interp)
{
    Free(interp->plug_name);
    Free(interp);
}

/*
 * Accessor functions for misc. configurable values.
 */

bool conf_get_use_tcp_wrappers(void)	        { return conf_use_tcp_wrap; }
void conf_set_use_tcp_wrappers(bool val)	{ conf_use_tcp_wrap = val; }
void conf_set_listen_port(int val)		{ conf_listen_port = val; }
int conf_get_listen_port(void)			{ return conf_listen_port; }

/*
 * vi:softtabstop=4
 */
