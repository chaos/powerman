/*****************************************************************************\ *  $Id:$
 *****************************************************************************
 *  Copyright (C) 2007-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
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
#include <stdio.h>
#include <libgen.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include "xtypes.h"
#include "xmalloc.h"
#include "error.h"
#include "argv.h"

#define DEFAULT_COMMUNITY "public"

#define OPTIONS "o:c:`"
static struct option longopts[] = {
        {"oid", required_argument, 0, 'o' },
        {"community", required_argument, 0, 'c' },
        {0,0,0,0},
};

void help(void)
{
    printf("Valid commands are:\n");
    printf("  get oid\n");
    printf("  set oid value\n");
}

void get(char **av, struct snmp_session *ss)
{
    struct snmp_pdu *pdu;
    struct snmp_pdu *response;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len = MAX_OID_LEN;
    struct variable_list *vars;
    int status;

    if (av[1] == NULL) {
        err (FALSE, "missing oid");
        return;
    }

    pdu = snmp_pdu_create (SNMP_MSG_GET);
    get_node(av[1], anOID, &anOID_len);
    snmp_add_null_var (pdu, anOID, anOID_len);
    status = snmp_synch_response (ss, pdu, &response);
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        for (vars = response->variables; vars; vars = vars->next_variable) {
            //print_variable (vars->name, vars->name_length, vars);
            switch (vars->type) {
                case ASN_OCTET_STR:
                    printf("%s: %*s\n", av[1], vars->val_len, vars->val.string);
                    break;
                case ASN_INTEGER:
                    printf("%s: %ld\n", av[1], *vars->val.integer);
                    break;
                default:
                    break;
            }
        }
    } else {
        if (status == STAT_SUCCESS)
            err_exit (FALSE, "error in packet: %s",
                      snmp_errstring (response->errstat));
        else
            snmp_sess_perror ("snmpget", ss);
    }
    if (response)
        snmp_free_pdu (response);
}

int docmd(char **av, struct snmp_session *ss)
{
    int rc = 0;

    if (av[0] != NULL) {
        if (strcmp(av[0], "help") == 0)
            help();
        else if (strcmp(av[0], "get") == 0)
            get(av, ss);
        else 
            printf("type \"help\" for a list of commands\n");
    }
    return rc;
}

void shell(struct snmp_session *ss)
{
    char buf[128];
    char **av;
    int rc = 0;

    while (rc == 0) {
        printf("snmppower> ");
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin)) {
            av = argv_create(buf, "");
            rc = docmd(av, ss);
            argv_destroy(av);
        } else
            rc = 1;
    }
}

void
usage(void)
{
    fprintf(stderr, "Usage: snmppower [--oid OID] --community COMMUNITY hostname\n");
    exit(1);
}

int
main(int argc, char *argv[])
{
    int c;
    struct snmp_session session, *ss;

    err_init(basename(argv[0]));

    init_snmp ("snmppower");
    snmp_sess_init (&session);
    session.version = SNMP_VERSION_1;
    session.community = (u_char *)DEFAULT_COMMUNITY;
    session.community_len = strlen (DEFAULT_COMMUNITY);

    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'o': /* --oid */
                //oid = xstrdup(optarg);
                break;
            case 'c': /* --community */
                session.community = (u_char *)optarg;
                session.community_len = strlen (optarg);
                break;
            default:
                usage();
                break;
        }
    }
    if (optind != argc - 1)
        usage();
    session.peername = argv[optind];

    if (!(ss = snmp_open (&session)))
        err_exit (FALSE, "snmp_open failed");

    shell(ss);

    snmp_close (ss); 

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
