/************************************************************\
 * Copyright (C) 2010 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <libgen.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include "xmalloc.h"
#include "error.h"
#include "argv.h"

#define OPTIONS "h:"
static struct option longopts[] = {
    {"hostname", required_argument, 0, 'h' },
    {0,0,0,0},
};

static void
get (char **av, struct snmp_session **ssp)
{
    struct snmp_pdu *pdu;
    struct snmp_pdu *response;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len = MAX_OID_LEN;
    struct variable_list *vars;
    int status;

    if (av[1] == NULL) {
        err (false, "missing oid");
        return;
    }
    if (*ssp == NULL) {
        err (false, "start session first");
        return;
    }
    pdu = snmp_pdu_create (SNMP_MSG_GET);
    if (!get_node (av[1], anOID, &anOID_len)) {
        snmp_free_pdu (pdu);
        printf ("error parsing oid\n");
        return;
    }
    snmp_add_null_var (pdu, anOID, anOID_len);
    status = snmp_synch_response (*ssp, pdu, &response);
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        for (vars = response->variables; vars; vars = vars->next_variable) {
            switch (vars->type) {
                case ASN_OCTET_STR:
                    printf("%s: %*s\n", av[1],
                           (int)vars->val_len, vars->val.string);
                    break;
                case ASN_INTEGER:
                    printf("%s: %ld\n", av[1], *vars->val.integer);
                    break;
                default:
                    print_variable (vars->name, vars->name_length, vars);
                    break;
            }
        }
    } else {
        if (status == STAT_SUCCESS)
            err_exit (false, "error in packet: %s",
                      snmp_errstring (response->errstat));
        else
            snmp_sess_perror ("snmpget", *ssp);
    }
    if (response)
        snmp_free_pdu (response);
}

static void
set (char **av, struct snmp_session **ssp)
{
    struct snmp_pdu *pdu;
    struct snmp_pdu *response;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len = MAX_OID_LEN;
    struct variable_list *vars;
    int status;

    if (av[1] == NULL) {
        err (false, "missing oid");
        return;
    }
    if (av[2] == NULL) {
        err (false, "missing type");
        return;
    }
    if (av[3] == NULL) {
        err (false, "missing value");
        return;
    }
    if (*ssp == NULL) {
        err (false, "start session first");
        return;
    }
    pdu = snmp_pdu_create (SNMP_MSG_SET);
    if (!get_node (av[1], anOID, &anOID_len)) {
        snmp_free_pdu (pdu);
        printf ("error parsing oid\n");
        return;
    }
    snmp_add_var (pdu, anOID, anOID_len, *(av[2]), av[3]);
    status = snmp_synch_response (*ssp, pdu, &response);
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        for (vars = response->variables; vars; vars = vars->next_variable) {
            switch (vars->type) {
                case ASN_OCTET_STR:
                    printf("%s: %*s\n", av[1],
                           (int)vars->val_len, vars->val.string);
                    break;
                case ASN_INTEGER:
                    printf("%s: %ld\n", av[1], *vars->val.integer);
                    break;
                default:
                    print_variable (vars->name, vars->name_length, vars);
                    break;
            }
        }
    } else {
        if (status == STAT_SUCCESS)
            err_exit (false, "error in packet: %s",
                      snmp_errstring (response->errstat));
        else
            snmp_sess_perror ("snmpset", *ssp);
    }
    if (response)
        snmp_free_pdu (response);
}

static void
start_v1v2c (char **av, int version, char *hostname, struct snmp_session **ssp)
{
    struct snmp_session session;

    if (av[1] == NULL) {
        err (false, "missing community");
        return;
    }
    if (*ssp) {
        err (false, "finish current session first");
        return;
    }
    snmp_sess_init (&session);
    session.version = version;
    session.community = (u_char *)xstrdup (av[1]);
    session.community_len = strlen (av[1]);
    session.peername = hostname;

    if (!(*ssp = snmp_open (&session))) {
        err (false, "snmp_open failed");
        xfree (session.community);
    }
}

static void
start_v3 (char **av, char *hostname, struct snmp_session **ssp)
{
    struct snmp_session session;

    if (av[1] == NULL) {
        err (false, "missing security name");
        return;
    }
    if (av[2] == NULL) {
        err (false, "missing passphrase");
        return;
    }
    if (strlen (av[2]) < 8) {
        err (false, "passphrase must be at least 8 characters");
        return;
    }
    snmp_sess_init (&session);
    session.version = SNMP_VERSION_3;
    session.peername = hostname;
    session.securityName = xstrdup (av[1]);
    session.securityNameLen = strlen (av[1]);

    session.securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;
    session.securityAuthProto = usmHMACMD5AuthProtocol;
    session.securityAuthProtoLen =
                              sizeof (usmHMACMD5AuthProtocol) / sizeof (oid);
    session.securityAuthKeyLen = USM_AUTH_KU_LEN;

    if (generate_Ku (session.securityAuthProto,
                     session.securityAuthProtoLen,
                     (u_char *)av[2], strlen (av[2]),
                     session.securityAuthKey,
                     &session.securityAuthKeyLen) != SNMPERR_SUCCESS) {
        err (false, "Error generating Ku from auth pass phrase");
    }

    if (!(*ssp = snmp_open (&session)))
        err (false, "snmp_open failed");
}

static void
mib (char **av, struct snmp_session **ssp)
{
    static int initialized = 0;

    if (av[1] == NULL) {
        err (false, "missing name");
        return;
    }
    if (!initialized) {
        init_mib ();
        initialized = 1;
    }
    netsnmp_read_module (av[1]); /* prints its own errors */
}

static void
finish (char **av, struct snmp_session **ssp)
{
    snmp_close (*ssp);
    *ssp = NULL;
}

static void
help (void)
{
    printf ("Valid commands are:\n");
    printf ("  start_v1 community\n");
    printf ("  start_v2c community\n");
    printf ("  start_v3 name passphrase\n");
    printf ("  mib name\n");
    printf ("  finish\n");
    printf ("  get oid\n");
    printf ("  set oid type value\n");
}

static int
docmd (char **av, char *hostname, struct snmp_session **ssp)
{
    int rc = 0;

    if (av[0] != NULL) {
        if (strcmp (av[0], "help") == 0)
            help ();
        else if (strcmp (av[0], "get") == 0)
            get (av, ssp);
        else if (strcmp (av[0], "set") == 0)
            set (av, ssp);
        else if (strcmp (av[0], "start_v1") == 0)
            start_v1v2c (av, SNMP_VERSION_1, hostname, ssp);
        else if (strcmp (av[0], "start_v2c") == 0)
            start_v1v2c (av, SNMP_VERSION_2c, hostname, ssp);
        else if (strcmp (av[0], "start_v3") == 0)
            start_v3 (av, hostname, ssp);
        else if (strcmp (av[0], "mib") == 0)
            mib (av, ssp);
        else if (strcmp (av[0], "finish") == 0)
            finish (av, ssp);
        else
            printf ("type \"help\" for a list of commands\n");
    }
    return rc;
}

static void
shell (char *hostname)
{
    char buf[128];
    char **av;
    int rc = 0;
    struct snmp_session *ss = NULL;

    while (rc == 0) {
        printf ("snmppower> ");
        fflush (stdout);
        if (fgets (buf, sizeof (buf), stdin)) {
            av = argv_create (buf, "");
            rc = docmd (av, hostname, &ss);
            argv_destroy (av);
        } else
            rc = 1;
    }
}

static void
usage (void)
{
    fprintf (stderr, "Usage: snmppower -h hostname\n");
    exit(1);
}

int
main (int argc, char *argv[])
{
    int c;
    char *hostname = NULL;

    err_init (basename(argv[0]));

    init_snmp ("snmppower");

    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'h':  /* --hostname */
                hostname = optarg;
                break;
            default:
                usage();
                break;
        }
    }
    if (optind != argc)
        usage ();
    if (hostname == NULL)
        usage ();

    shell(hostname);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
