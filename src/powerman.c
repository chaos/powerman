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

#define _GNU_SOURCE             /* for dprintf */
#include <stdio.h>
#include <string.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <libgen.h>

#include "wrappers.h"
#include "error.h"
#include "hostlist.h"
#include "client_proto.h"

#define SERVER_HOSTNAME "localhost"
#define SERVER_PORT     "10101"

static void _connect_to_server(char *host, char *port);
static void _disconnect_from_server(void);
static void _usage(void);
static void _license(void);
static void _version(void);
static int _getline(void);
static void _expect(char *str);
static int _process_response(void);

static int server_fd = -1;

#define OPT_STRING "01crlqfubntLd:VD"
static const struct option long_options[] = {
    {"on", no_argument, 0, '1'},
    {"off", no_argument, 0, '0'},
    {"cycle", no_argument, 0, 'c'},
    {"reset", no_argument, 0, 'r'},
    {"list", no_argument, 0, 'l'},
    {"query", no_argument, 0, 'q'},
    {"flash", no_argument, 0, 'f'},
    {"unflash", no_argument, 0, 'u'},
    {"beacon", no_argument, 0, 'b'},
    {"node", no_argument, 0, 'n'},
    {"temp", no_argument, 0, 't'},
    {"license", no_argument, 0, 'L'},
    {"destination", required_argument, 0, 'd'},
    {"version", no_argument, 0, 'V'},
    {"device", no_argument, 0, 'D'},
    {0, 0, 0, 0}
};

static const struct option *longopts = long_options;

int main(int argc, char **argv)
{
    int c;
    int longindex;
    hostlist_t targets;
    bool have_targets = FALSE;
    char targstr[CP_LINEMAX];
    int res = 0;
    char *prog;
    enum { CMD_NONE, CMD_ON, CMD_OFF, CMD_LIST, CMD_CYCLE, CMD_RESET,
        CMD_QUERY, CMD_FLASH, CMD_UNFLASH, CMD_BEACON, CMD_TEMP, CMD_NODE,
        CMD_DEVICE
    } cmd = CMD_NONE;
    char *port = NULL;
    char *host = NULL;

    prog = basename(argv[0]);
    err_init(prog);

    if (strcmp(prog, "on") == 0)
        cmd = CMD_ON;
    else if (strcmp(prog, "off") == 0)
        cmd = CMD_OFF;

    /*
     * Parse options.
     */
    opterr = 0;
    while ((c = getopt_long(argc, argv, OPT_STRING, longopts,
                        &longindex)) != -1) {
        switch (c) {
        case 'l':              /* --list */
            cmd = CMD_LIST;
            break;
        case '1':              /* --on */
            cmd = CMD_ON;
            break;
        case '0':              /* --off */
            cmd = CMD_OFF;
            break;
        case 'c':              /* --cycle */
            cmd = CMD_CYCLE;
            break;
        case 'r':              /* --reset */
            cmd = CMD_RESET;
            break;
        case 'q':              /* --query */
            cmd = CMD_QUERY;
            break;
        case 'f':              /* --flash */
            cmd = CMD_FLASH;
            break;
        case 'u':              /* --unflash */
            cmd = CMD_UNFLASH;
            break;
        case 'b':              /* --beacon */
            cmd = CMD_BEACON;
            break;
        case 'n':              /* --node */
            cmd = CMD_NODE;
            break;
        case 't':              /* --temp */
            cmd = CMD_TEMP;
            break;
        case 'D':              /* --device */
            cmd = CMD_DEVICE;
            break;
        case 'd':              /* --destination host[:port] */
            if ((port = strchr(optarg, ':')))
                *port++ = '\0';  
            host = optarg;
            break;
        case 'L':              /* --license */
            _license();
            /*NOTREACHED*/
            break;
        case 'V':              /* --version */
            _version();
            /*NOTREACHED*/
            break;
        default:
            _usage();
            /*NOTREACHED*/
            break;
        }
    }

    if (cmd == CMD_NONE)
        _usage();

    /* remaining arguments used to build a single hostlist argument */
    while (optind < argc) {
        if (!have_targets) {
            targets = hostlist_create(NULL);
            have_targets = TRUE;
        }
        hostlist_push(targets, argv[optind]);
        optind++;
    }
    if (have_targets) {
        hostlist_sort(targets);
        hostlist_ranged_string(targets, sizeof(targstr), targstr);
    }

    _connect_to_server(host ? host : SERVER_HOSTNAME, 
                       port ? port : SERVER_PORT);

    /*
     * Execute the commands.
     */
    switch (cmd) {
    case CMD_LIST:
        if (have_targets)
            err_exit(FALSE, "option does not accept targets");
        dprintf(server_fd, CP_NODES CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_QUERY:
        if (have_targets)
            dprintf(server_fd, CP_STATUS CP_EOL, targstr);
        else
            dprintf(server_fd, CP_STATUS_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_ON:
        if (!have_targets)
            err_exit(FALSE, "option requires targets");
        dprintf(server_fd, CP_ON CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_OFF:
        if (!have_targets)
            err_exit(FALSE, "option requires targets");
        dprintf(server_fd, CP_OFF CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_RESET:
        if (!have_targets)
            err_exit(FALSE, "option requires targets");
        dprintf(server_fd, CP_RESET CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_CYCLE:
        if (!have_targets)
            err_exit(FALSE, "option requires targets");
        dprintf(server_fd, CP_CYCLE CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_FLASH:
        if (!have_targets)
            err_exit(FALSE, "option requires targets");
        dprintf(server_fd, CP_BEACON_ON CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_UNFLASH:
        if (!have_targets)
            err_exit(FALSE, "option requires targets");
        dprintf(server_fd, CP_BEACON_OFF CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_BEACON:
        if (have_targets)
            dprintf(server_fd, CP_BEACON CP_EOL, targstr);
        else
            dprintf(server_fd, CP_BEACON_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_TEMP:
        if (have_targets)
            dprintf(server_fd, CP_TEMP CP_EOL, targstr);
        else
            dprintf(server_fd, CP_TEMP_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_NODE:
        if (have_targets)
            dprintf(server_fd, CP_SOFT CP_EOL, targstr);
        else
            dprintf(server_fd, CP_SOFT_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_DEVICE:
        if (have_targets)
            dprintf(server_fd, CP_DEVICE CP_EOL, targstr);
        else
            dprintf(server_fd, CP_DEVICE_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_NONE:
    default:
        _usage();
    }

    _disconnect_from_server();

    exit(res);
}

/*
 * Display powerman usage and exit.
 */
static void _usage(void)
{
    printf("Usage: %s [OPTIONS] [TARGETS]\n", "powerman");
    printf(
 "-1 --on        Power on targets      -0 --off       Power off targets\n"
 "-q --query     Query plug status     -l --list      List available targets\n"
 "-c --cycle     Power cycle targets   -r --reset     Reset targets\n"
 "-f --flash     Turn beacon on        -u --unflash   Turn beacon off\n"
 "-b --beacon    Query beacon status   -n --node      Query node status\n"
 "-t --temp      Query temperature     -V --version   Report powerman version\n"
  );
    exit(1);
}


/*
 * Display powerman license and exit.
 */
static void _license(void)
{
    printf(
 "Copyright (C) 2001-2002 The Regents of the University of California.\n"
 "Produced at Lawrence Livermore National Laboratory.\n"
 "Written by Andrew Uselton <uselton2@llnl.gov>.\n"
 "http://www.llnl.gov/linux/powerman/\n"
 "UCRL-CODE-2002-008\n\n"
 "PowerMan is free software; you can redistribute it and/or modify it\n"
 "under the terms of the GNU General Public License as published by\n"
 "the Free Software Foundation.\n");
    exit(1);
}

/*
 * Display powerman version and exit.
 */
static void _version(void)
{
    printf("%s\n", 
            strlen(POWERMAN_VERSION) > 0 ? POWERMAN_VERSION : "development");
    exit(1);
}

/*
 * Set up connection to server and get to the command prompt.
 */
static void _connect_to_server(char *host, char *port)
{
    struct addrinfo hints, *addrinfo;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    Getaddrinfo(host, port, &hints, &addrinfo);

    server_fd = Socket(addrinfo->ai_family, addrinfo->ai_socktype,
                       addrinfo->ai_protocol);

    Connect(server_fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
    freeaddrinfo(addrinfo);

    _expect(CP_VERSION);
    _expect(CP_PROMPT);
}

static void _disconnect_from_server(void)
{
    dprintf(server_fd, CP_QUIT CP_EOL);
    _expect(CP_RSP_QUIT);
}

/* 
 * Get a line from the socket and display on stdout.
 * Return the numerical portion of the repsonse.
 */
static int _getline(void)
{
    char buf[CP_LINEMAX];
    int size = CP_LINEMAX;
    char *p = buf;
    int num;

    while (size > 1) {          /* leave room for terminating null */
        if (Read(server_fd, p, 1) <= 0)
            err_exit(TRUE, "lost connection with server\n");
        if (*p == '\r')
            continue;
        if (*p == '\n')
            break;
        size--;
        p++;
    }
    *p = '\0';
    if (strlen(buf) > 4)
        printf("%s\n", buf + 4);
    else
        err_exit(FALSE, "unexpected response from server\n");
    num = strtol(buf, (char **) NULL, 10);
    return (num == LONG_MIN || num == LONG_MAX) ? -1 : num;
}

static int _process_response(void)
{
    int num;

    do {
        num = _getline();
    } while (!CP_IS_ALLDONE(num));
    return (CP_IS_FAILURE(num) ? num : 0);
}

/* 
 * Read strlen(str) bytes from file descriptor.
 * If it doesn't match what we're expecting, it's probably an error.
 */
static void _expect(char *str)
{
    char buf[CP_LINEMAX];
    int res;

    assert(strlen(str) < sizeof(buf));
    res = Read(server_fd, buf, strlen(str));
    if (res < 0)
        err_exit(TRUE, "lost connection with server\n");
    if (res != strlen(str))
        err_exit(FALSE, "short read\n");
    buf[res] = '\0';
    if (strcmp(str, buf) != 0)
        err_exit(FALSE, "%s\n", buf);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
