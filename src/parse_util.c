/*****************************************************************************\
 *  $Id$
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "list.h"
#include "hostlist.h"
#include "xtypes.h"
#include "error.h"
#include "parse_util.h"
#include "xmalloc.h"
#include "xpoll.h"
#include "pluglist.h"
#include "client.h"
#include "powerman.h"

typedef struct {
    char *name;
    hostlist_t hl;
} alias_t;

static bool         conf_use_tcp_wrap = FALSE;
static int          conf_listen_port;
static hostlist_t   conf_nodes = NULL;
static List         conf_aliases = NULL;    /* list of alias_t's */

static bool _validate_config(void);
static void _alias_destroy(alias_t *a);

extern int parse_config_file(char *filename); /* yacc/lex parser */

/* 
 * initialize module
 * parse the named config file - on error, exit with a message to stderr 
 */
void conf_init(char *filename)
{
    struct stat stbuf;
    bool valid;

    conf_listen_port = strtol(DFLT_PORT, NULL, 10);

    conf_nodes = hostlist_create(NULL);

    conf_aliases = list_create((ListDelF) _alias_destroy);

    /* validate config file */
    if (stat(filename, &stbuf) < 0)
        err_exit(TRUE, "%s", filename);
    if ((stbuf.st_mode & S_IFMT) != S_IFREG)
        err_exit(FALSE, "%s is not a regular file\n", filename);

    /* 
     * Call yacc parser against config file.  The parser calls support 
     * functions below and builds 'dev_devices' (devices.c),
     * 'conf_cluster', 'conf_specs' (config.c), and various other 
     * conf_* attributes (config.c).
     */
    parse_config_file(filename);

    valid = _validate_config();

    if (!valid)
        exit(1);
}

/* finalize module */
void conf_fini(void)
{
    if (conf_nodes != NULL)
        hostlist_destroy(conf_nodes);
}

/*
 * Check the config file and exit with error if any problems are found.
 */
static bool _validate_config(void)
{
    ListIterator itr;
    bool valid = TRUE;
    alias_t *a;

    /* make sure aliases do not point to bogus node names */
    itr = list_iterator_create(conf_aliases);
    while ((a = list_next(itr)) != NULL) {
        hostlist_iterator_t hitr = hostlist_iterator_create(a->hl);
        char *host;

        if (hitr == NULL)
            err_exit(FALSE, "hostlist_iterator_create failed");
        while ((host = hostlist_next(hitr)) != NULL) {
            if (!conf_node_exists(host)) {
                err(FALSE, "alias '%s' references nonexistant node '%s'",
                        a->name, host);
                valid = FALSE;
                free(host);
                break;
            } else 
                free(host);
        }
        hostlist_iterator_destroy(hitr);
    }
    list_iterator_destroy(itr);

    return valid;
}

/*
 * Node conf_nodes list.
 */

bool conf_node_exists(char *node)
{
    int res;

    res = hostlist_find(conf_nodes, node);
    return (res == -1 ? FALSE : TRUE);
}

bool conf_addnodes(char *nodelist)
{
    hostlist_t hl = hostlist_create(nodelist);
    hostlist_iterator_t itr = hostlist_iterator_create(hl);
    char *node;
    int res = TRUE;

    while ((node = hostlist_next(itr))) {
        if (conf_node_exists(node)) {
            free(node);
            res = FALSE;
            break; 
        } else {
            hostlist_push(conf_nodes, node);
            free(node);
        }
    }
    hostlist_iterator_destroy(itr);
    hostlist_destroy(hl);

    return res;
}

hostlist_t conf_getnodes(void)
{
    return conf_nodes;
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
#if ! HAVE_TCP_WRAPPERS
    if (val == TRUE)
        err_exit(FALSE, "powerman was not built with tcp_wrapper support");
#endif
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

/* Expand any aliases present in hostlist.  
 * N.B. Aliases cannot contain other aliases.
 */
void conf_exp_aliases(hostlist_t hl)
{
    hostlist_iterator_t itr = NULL;
    hostlist_t newhosts = hostlist_create(NULL);
    char *host;
  
    /* Put the expansion of any aliases in the hostlist into 'newhosts', 
     * deleting the original reference from the hostlist.
     */
    if (newhosts == NULL)
        err_exit(FALSE, "hostlist_create failed");
    if ((itr = hostlist_iterator_create(hl)) == NULL)
        err_exit(FALSE, "hostlist_iterator_create failed");
    while ((host = hostlist_next(itr)) != NULL) {
        alias_t *a;
            
        a = list_find_first(conf_aliases, (ListFindF) _alias_match, host);
        if (a) {
            hostlist_delete_host(hl, host); 
            hostlist_push_list(newhosts, a->hl);
            hostlist_iterator_reset(itr); /* not sure of itr position after
                                             insertion/deletion so reset */
        }
        free(host);
    }
    hostlist_iterator_destroy(itr);

    /* dump the contents of 'newhosts' into the hostlist */
    hostlist_push_list(hl, newhosts);
    hostlist_destroy(newhosts);
}

static void _alias_destroy(alias_t *a)
{
    if (a->name)
        xfree(a->name);
    if (a->hl)
        hostlist_destroy(a->hl);
    xfree(a);
}

static alias_t *_alias_create(char *name, char *hosts)
{
    alias_t *a = NULL;

    if (!list_find_first(conf_aliases, (ListFindF) _alias_match, name)) {
        a = (alias_t *)xmalloc(sizeof(alias_t));
        a->name= xstrdup(name);
        a->hl = hostlist_create(hosts);
        if (a->hl == NULL) {
            _alias_destroy(a);
            a = NULL;
        }
    }
    return a;
}

/*
 * Called from the parser.
 */
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
