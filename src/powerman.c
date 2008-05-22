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
#include <stdio.h>
#include <string.h>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <libgen.h>
#include <limits.h>

#include "powerman.h"
#include "wrappers.h"
#include "error.h"
#include "hostlist.h"
#include "client_proto.h"
#include "debug.h"

static void _connect_to_server(char *host, char *port);
static void _disconnect_from_server(void);
static void _usage(void);
static void _license(void);
static void _version(void);
static void _getline(char *str, int len);
static int _process_line(void);
static void _expect(char *str);
static int _process_response(void);
static void _process_version(void);

static int server_fd = -1;

#define OPT_STRING "01crlqfubntLd:VDvxT"
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
    {"telemetry", no_argument, 0, 'T'},
    {"exprange", no_argument, 0, 'x'},
    {0, 0, 0, 0}
};

static const struct option *longopts = long_options;

int main(int argc, char **argv)
{
    int c;
    int longindex;
    hostlist_t targets = NULL;
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
    bool telemetry = FALSE;
    bool exprange = FALSE;

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
        case 'T':              /* --telemetry */
            telemetry = TRUE;
            break;
        case 'x':              /* --exprange */
            exprange = TRUE;
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
            if ((targets = hostlist_create(NULL)) == NULL)
                err_exit(FALSE, "hostlist error");
            have_targets = TRUE;
        }
        if (hostlist_push(targets, argv[optind]) == 0)
            err_exit(FALSE, "hostlist error");
        optind++;
    }
    if (have_targets) {
        hostlist_sort(targets);
        if (hostlist_ranged_string(targets, sizeof(targstr), targstr) == -1)
            err_exit(FALSE, "hostlist error");
    }

    /* verify a few option requirements */
    switch (cmd) {
    case CMD_LIST:
        if (have_targets)
            err_exit(FALSE, "option does not accept targets");
        break;
    case CMD_ON:
    case CMD_OFF:
    case CMD_RESET:
    case CMD_CYCLE:
    case CMD_FLASH:
    case CMD_UNFLASH:
        if (!have_targets)
            err_exit(FALSE, "option requires targets");
        break;
    default:
        break;
    }

    _connect_to_server(host ? host : DFLT_HOSTNAME, port ? port : DFLT_PORT);

    if (telemetry) {
        Dprintf(server_fd, CP_TELEMETRY CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        if (res != 0)
            goto done;
    }

    if (exprange) {
        Dprintf(server_fd, CP_EXPRANGE CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        if (res != 0)
            goto done;
    }

    /*
     * Execute the commands.
     */
    switch (cmd) {
    case CMD_LIST:
        Dprintf(server_fd, CP_NODES CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_QUERY:
        if (have_targets)
            Dprintf(server_fd, CP_STATUS CP_EOL, targstr);
        else
            Dprintf(server_fd, CP_STATUS_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_ON:
        Dprintf(server_fd, CP_ON CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_OFF:
        Dprintf(server_fd, CP_OFF CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_RESET:
        Dprintf(server_fd, CP_RESET CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_CYCLE:
        Dprintf(server_fd, CP_CYCLE CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_FLASH:
        Dprintf(server_fd, CP_BEACON_ON CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_UNFLASH:
        Dprintf(server_fd, CP_BEACON_OFF CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_BEACON:
        if (have_targets)
            Dprintf(server_fd, CP_BEACON CP_EOL, targstr);
        else
            Dprintf(server_fd, CP_BEACON_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_TEMP:
        if (have_targets)
            Dprintf(server_fd, CP_TEMP CP_EOL, targstr);
        else
            Dprintf(server_fd, CP_TEMP_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_NODE:
        if (have_targets)
            Dprintf(server_fd, CP_SOFT CP_EOL, targstr);
        else
            Dprintf(server_fd, CP_SOFT_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_DEVICE:
        if (have_targets)
            Dprintf(server_fd, CP_DEVICE CP_EOL, targstr);
        else
            Dprintf(server_fd, CP_DEVICE_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_NONE:
    default:
        _usage();
    }

done:
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
 "-D --device    Report device status  -T --telemetry Show device telemetry\n"
 "-x --exprange  Expand host ranges\n"
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
    printf("%s\n", VERSION);
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

    if (Connect(server_fd, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0) {
        /* EINPROGRESS (not possible) | ECONNREFUSED */
        err_exit(TRUE, "powermand");
    }
    freeaddrinfo(addrinfo);

    _process_version();
    _expect(CP_PROMPT);
}

static void _disconnect_from_server(void)
{
    Dprintf(server_fd, CP_QUIT CP_EOL);
    _expect(CP_RSP_QUIT);
}

/*
 * Return true if response should be suppressed.
 */
static bool _supress(int num)
{
    char *ignoreme[] = { CP_RSP_QRY_COMPLETE, CP_RSP_TELEMETRY, 
        CP_RSP_EXPRANGE };
    bool res = FALSE;
    int i;

    for (i = 0; i < sizeof(ignoreme)/sizeof(char *); i++) {
        if (strtol(ignoreme[i], (char **) NULL, 10) == num) {
            res = TRUE;
            break;
        }
    }

    return res;
}

/* 
 * Read a line of data terminated with \r\n or just \n.
 */
static void _getline(char *buf, int size)
{
    while (size > 1) {          /* leave room for terminating null */
        if (Read(server_fd, buf, 1) <= 0)
            err_exit(TRUE, "lost connection with server");
        if (*buf == '\r')
            continue;
        if (*buf == '\n')
            break;
        size--;
        buf++;
    }
    *buf = '\0';
}

/* 
 * Get a line from the socket and display on stdout.
 * Return the numerical portion of the repsonse.
 */
static int _process_line(void)
{
    char buf[CP_LINEMAX];
    long int num;

    _getline(buf, CP_LINEMAX);

    num = strtol(buf, (char **) NULL, 10);
    if (num == LONG_MIN || num == LONG_MAX)
        num = -1;
    if (strlen(buf) > 4) {
        if (!_supress(num))
            printf("%s\n", buf + 4);
    } else
        err_exit(FALSE, "unexpected response from server");
    return num;
}

/*
 * Read version and warn if it doesn't match the client's.
 */
static void _process_version(void)
{
    char buf[CP_LINEMAX], vers[CP_LINEMAX];

    _getline(buf, CP_LINEMAX);
    if (sscanf(buf, CP_VERSION, vers) != 1)
        err_exit(FALSE, "unexpected response from server");
    if (strcmp(vers, VERSION) != 0)
        err(FALSE, "warning: server version (%s) != client (%s)",
                vers, VERSION);
}

static int _process_response(void)
{
    int num;

    do {
        num = _process_line();
    } while (!CP_IS_ALLDONE(num));
    return (CP_IS_FAILURE(num) ? num : 0);
}

/* 
 * Read strlen(str) bytes from file descriptor and exit if
 * it doesn't match 'str'.
 */
static void _expect(char *str)
{
    char buf[CP_LINEMAX];
    int len = strlen(str);
    char *p = buf;
    int res;

    assert(len < sizeof(buf));
    do {
        res = Read(server_fd, p, len);
        if (res < 0)
            err_exit(TRUE, "lost connection with server");
        p += res;
        *p = '\0';
        len -= res;
    } while (strcmp(str, buf) != 0 && len > 0);

    /* Shouldn't happen.  We are not handling the general case of the server
     * returning the wrong response.  Read() loop above may hang in that case.
     */
    if (strcmp(str, buf) != 0)
        err_exit(FALSE, "unexpected response from server");
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
