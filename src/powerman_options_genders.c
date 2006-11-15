/*****************************************************************************\
 *  $Id: powerman.c 758 2006-10-24 06:21:03Z achu $
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
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <genders.h>

#include "powerman_options.h"
#include "hostlist.h"

static char *gquery = NULL;

struct powerman_option genders_options[] = {
  {
    'g',
    "gquery",
    "gquery",
    "Specify nodes via genders query",
    POWERMAN_OPTION_TYPE_GET_NODES
  },
  {
    0,
    NULL,
    NULL,
    NULL,
    0
  }
};

static int
genders_options_setup(void)
{
    return 0;
}

static int
genders_options_cleanup(void)
{
    return 0;
}

static int
genders_options_process_option(char c, char *optarg)
{
    if (c != 'g')
        return -1;

    gquery = optarg;
    return 0;
}

static int
genders_options_get_nodes(char *buf, int buflen)
{
    genders_t handle = NULL;
    hostlist_t h = NULL;
    char **nodes;
    int nodeslen;
    int nodesgot;
    int i;
    
    if (!gquery || !buf || !buflen)
        return -1;

    memset(buf, '\0', buflen);

    if (!(handle = genders_handle_create())) {
        fprintf(stderr, "genders_options_get_nodes: genders_handle_create()\n");
        exit(1);
    }

    if (!(h = hostlist_create(NULL))) {
        fprintf(stderr, "genders_options_get_nodes: hostlist_create()\n");
        exit(1);
    }

    if (genders_load_data(handle, NULL) < 0) {
        fprintf(stderr, "genders_options_get_nodes: genders_load_data(): %s\n",
                genders_errormsg(handle));
        exit(1);
    }

    if ((nodeslen = genders_nodelist_create(handle, &nodes)) < 0) {
        fprintf(stderr, "genders_options_get_nodes: genders_nodelist_create() : %s\n",
                genders_errormsg(handle));
        exit(1);
    }

    if ((nodesgot = genders_query(handle,
                                  nodes,
                                  nodeslen,
                                  gquery)) < 0) {
        fprintf(stderr, "genders_options_get_nodes: genders_query() : %s\n",
                genders_errormsg(handle));
        exit(1);
    }

    if (!nodesgot) {
        fprintf(stderr, "genders_query(): no nodes found\n");
        exit(1);
    }

    for (i = 0; i < nodesgot; i++) {
        if (!hostlist_push(h, nodes[i])) {
            fprintf(stderr, "genders_options_get_nodes: hostlist_push()\n");
            exit(1);
        }
    }

    if (hostlist_ranged_string(h, buflen, buf) < 0) {
        fprintf(stderr, "genders_options_get_nodes: hostlist_ranged_string()\n");
        exit(1);
    }

    (void)genders_handle_destroy(handle);
    (void)hostlist_destroy(h);
    return 0;
}

#if WITH_STATIC_MODULES
struct powerman_options_module_info genders_module_info = 
#else  /* !WITH_STATIC_MODULES */
struct powerman_options_module_info powerman_module_info = 
#endif /* !WITH_STATIC_MODULES */
  {
    "genders",
    &genders_options[0],
    genders_options_setup,
    genders_options_cleanup,
    genders_options_process_option,
    genders_options_get_nodes,
  };
