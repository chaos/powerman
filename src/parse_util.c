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
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "powerman.h"
#include "error.h"
#include "list.h"
#include "parse_util.h"
#include "device.h"
#include "wrappers.h"
#include "client.h"

static int _spec_match(Spec * spec, void *key);
static void _spec_destroy(Spec * spec);

/* 
 * Device specifications only exist during parsing.
 * After that their contents are copied for each instantiation of the device and
 * the original specification is not used.
 */
static List tmp_specs = NULL;

/*
 * Some configurable values - accessor functions defined below.
 */
static bool conf_use_tcp_wrap = FALSE;
static int conf_listen_port;
static hostlist_t conf_nodes = NULL;

typedef struct {
    char *name;
    hostlist_t hl;
} alias_t;

static List conf_aliases = NULL; /* list of alias_t's */

static void _alias_destroy(alias_t *a);

/* 
 * initialize module
 * parse the named config file - on error, exit with a message to stderr 
 */
void conf_init(char *filename)
{
    struct stat stbuf;
    int parse_config_file(char *filename);

    conf_listen_port = strtol(DFLT_PORT, NULL, 10);

    conf_nodes = hostlist_create(NULL);

    tmp_specs = list_create((ListDelF) _spec_destroy);

    conf_aliases = list_create((ListDelF) _alias_destroy);

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
void conf_fini(void)
{
    if (conf_nodes != NULL)
        hostlist_destroy(conf_nodes);
}

/*******************************************************************
 *                                                                 *
 * Stmt                                                       *
 *   A Stmt structure is an element in a script.  The script  *
 * is a list of these elemments that implements one of the actions *
 * that clients may request of devices.                            *
 *                                                                 *
 *******************************************************************/

Stmt *conf_stmt_create(StmtType type, char *s1,
                                 List map, struct timeval tv)
{
    Stmt *stmt;
    reg_syntax_t cflags = REG_EXTENDED;

    stmt = (Stmt *) Malloc(sizeof(Stmt));
    stmt->type = type;
    switch (type) {
    case STMT_SEND:
        stmt->u.send.fmt = Strdup(s1);
        break;
    case STMT_EXPECT:
        Regcomp(&(stmt->u.expect.exp), s1, cflags);
        stmt->u.expect.map = map;
        break;
    case STMT_DELAY:
        stmt->u.delay.tv = tv;
        break;
    default:
    }
    return stmt;
}

void conf_stmt_destroy(Stmt * stmt)
{
    assert(stmt != NULL);

    switch (stmt->type) {
    case STMT_SEND:
        Free(stmt->u.send.fmt);
        break;
    case STMT_EXPECT:
        if (stmt->u.expect.map != NULL)
            list_destroy(stmt->u.expect.map);
        break;
    case STMT_DELAY:
    default:
    }
    Free(stmt);
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
    timerclear(&spec->timeout);
    timerclear(&spec->ping_period);
    for (i = 0; i < NUM_SCRIPTS; i++)
        spec->prescripts[i] = NULL;
    return spec;
}

/* list 'match' function for conf_find_spec() */
static int _spec_match(Spec * spec, void *key)
{
    return (strcmp(spec->name, (char *) key) == 0);
}

/* list destructor for tmp_specs */
static void _spec_destroy(Spec * spec)
{
    int i;

    Free(spec->name);
    Free(spec->off);
    Free(spec->on);

    for (i = 0; i < spec->size; i++)
        Free(spec->plugname[i]);
    Free(spec->plugname);
    for (i = 0; i < NUM_SCRIPTS; i++)
        if (spec->prescripts[i])
            list_destroy(spec->prescripts[i]);
    Free(spec);
}

/* add a Spec to the internal list */
void conf_add_spec(Spec * spec)
{
    assert(tmp_specs != NULL);
    list_append(tmp_specs, spec);
}

/* find and return a Spec from the internal list */
Spec *conf_find_spec(char *name)
{
    return list_find_first(tmp_specs, (ListFindF) _spec_match, name);
}

/*******************************************************************
 *                                                                 *
 * PreStmt                                                         *
 *   A PreStmt structure is an object associated with a Spec       *
 * that holds information for a Stmt.  The Stmt itself   *
 * cannot be constructed unitl it is added to the device's         *
 * protocol.  All the infromation the Stmt needs at that time *
 * is present in the PreStmt.                                      *
 *                                                                 *
 *******************************************************************/

PreStmt *conf_prestmt_create(StmtType type, char *str1,
                             struct timeval * tv, List map)
{
    PreStmt *specl;

    specl = (PreStmt *) Malloc(sizeof(PreStmt));
    memset(specl, 0, sizeof(PreStmt));
    specl->type = type;
    switch (type) {
    case STMT_SEND:
    case STMT_EXPECT:
        assert(str1 != NULL && tv == NULL);
        specl->string1 = Strdup(str1);
        break;
    case STMT_DELAY:
        assert(str1 == NULL && tv != NULL);
        specl->tv = *tv;
        break;
    default:
        assert(FALSE);
    }
    specl->map = map;
    return specl;
}

void conf_prestmt_destroy(PreStmt * specl)
{
    if (specl->string1 != NULL)
        Free(specl->string1);
    specl->string1 = NULL;
    if (specl->map != NULL)
        list_destroy(specl->map);
    specl->map = NULL;
    Free(specl);
}

bool conf_node_exists(char *node)
{
    int res;

    res = hostlist_find(conf_nodes, node);
    return (res == -1 ? FALSE : TRUE);
}

bool conf_addnode(char *node)
{
#if 1 
    if (conf_node_exists(node))
        return FALSE;
#else
    /* redundant PS support: permit duplicate entries for node name */
#endif
    hostlist_push(conf_nodes, node);
    return TRUE;
}

hostlist_t conf_getnodes(void)
{
    return conf_nodes;
}

/*******************************************************************
 *                                                                 *
 * Interp                                                  *
 *   An Interp structure is the means by which an EXPECT   *
 * script element identifies the pieces of the RegEx matched in an *
 * may be interpreted to give state information.                   *
 *                                                                 *
 *******************************************************************/

Interp *conf_interp_create(char *name)
{
    Interp *interp;

    interp = (Interp *) Malloc(sizeof(Interp));
    interp->plug_name = Strdup(name);
    interp->match_pos = -1;
    return interp;
}

void conf_interp_destroy(Interp * interp)
{
    if (interp->plug_name)
        Free(interp->plug_name);
    Free(interp);
}

/*
 * Accessor functions for misc. configurable values.
 */

bool conf_get_use_tcp_wrappers(void)
{
    return conf_use_tcp_wrap;
}

void conf_set_use_tcp_wrappers(bool val)
{
    conf_use_tcp_wrap = val;
}
void conf_set_listen_port(int val)
{
    conf_listen_port = val;
}
int conf_get_listen_port(void)
{
    return conf_listen_port;
}

/*
 * Manage a list of nodename aliases.
 */

static int _alias_match(alias_t *a, char *name)
{
    return (strcmp(a->name, name) == 0);
}

void conf_exp_aliases(hostlist_t hl)
{
    hostlist_iterator_t itr = NULL;
    char *host;
    
    if ((itr = hostlist_iterator_create(hl)) == NULL) {
        err(FALSE, "conf_exp_aliases: error craeting hostlist iterator");
        return;
    }

    while ((host = hostlist_next(itr)) != NULL) {
        alias_t *a;
            
        a = list_find_first(conf_aliases, (ListFindF) _alias_match, host);
        if (a) {
            hostlist_delete_host(hl, host); 
            hostlist_push_list(hl, a->hl);
            hostlist_iterator_reset(itr);
        }
    }
    hostlist_iterator_destroy(itr);
}

static void _alias_destroy(alias_t *a)
{
    if (a->name)
        Free(a->name);
    if (a->hl)
        hostlist_destroy(a->hl);
    Free(a);
}

static alias_t *_alias_create(char *name, char *hosts)
{
    alias_t *a = NULL;

    if (!list_find_first(conf_aliases, (ListFindF) _alias_match, name)) {
        a = (alias_t *)Malloc(sizeof(alias_t));
        a->name= Strdup(name);
        a->hl = hostlist_create(hosts);
        if (a->hl == NULL) {
            _alias_destroy(a);
            a = NULL;
        }
    }
    return a;
}

bool conf_add_alias(char *name, char *hosts)
{
    alias_t *a;

    if ((a = _alias_create(name, hosts))) {
        list_push(conf_aliases, a);
        return TRUE;
    }
    return FALSE;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
