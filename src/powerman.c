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

#define _GNU_SOURCE  /* for dprintf */
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
#define SERVER_PORT	"10101"

static void _connect_to_server(void);
static void _disconnect_from_server(void);
static void _usage(void);
static int _getline(void);
static void _expect(char *str);
static int _process_response(void);

static int server_fd = -1;

#define OPT_STRING "01crlq"
static const struct option long_options[] = {
    {"on", no_argument, 0, '1'},
    {"off", no_argument, 0, '0'},
    {"cycle", no_argument, 0, 'c'},
    {"reset", no_argument, 0, 'r'},
    {"list", no_argument, 0, 'l'},
    {"query", no_argument, 0, 'q'},
    {0, 0, 0, 0}
};

static const struct option *longopts = long_options;

const char *powerman_license =
    "Copyright (C) 2001-2002 The Regents of the University of California.\n"
    "Produced at Lawrence Livermore National Laboratory.\n"
    "Written by Andrew Uselton <uselton2@llnl.gov>.\n"
    "http://www.llnl.gov/linux/powerman/\n"
    "UCRL-CODE-2002-008\n\n"
    "PowerMan is free software; you can redistribute it and/or modify it\n"
    "under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation.\n";

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
	    CMD_REPORT } cmd = CMD_NONE;

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
    while ((c = getopt_long(argc, argv,OPT_STRING,longopts,&longindex)) != -1) {
	switch (c) {
	    case 'l':	/* --list */
		cmd = CMD_LIST;
		break;
	    case '1':	/* --on */
		cmd = CMD_ON;
		break;
	    case '0':	/* --off */
		cmd = CMD_OFF;
		break;
	    case 'c':	/* --cycle */
		cmd = CMD_CYCLE;
		break;
	    case 'r':	/* --reset */
		cmd = CMD_RESET;
		break;
	    case 'q':	/* --query */
		cmd = CMD_REPORT;
		break;
	    default:
		_usage();
		break;
	}
    }
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

    _connect_to_server();

    /*
     * Execute the commands.
     */
    switch (cmd) {
	case CMD_LIST:
	    if (have_targets)
		_usage();
	    dprintf(server_fd, CP_NODES CP_EOL);
	    res = _process_response();
	    _expect(CP_PROMPT);
	    break;
	case CMD_REPORT:
	    if (have_targets)
		dprintf(server_fd, CP_STATUS CP_EOL, targstr);
	    else
		dprintf(server_fd, CP_STATUS_ALL CP_EOL);
	    res = _process_response();
	    _expect(CP_PROMPT);
	    break;
	case CMD_ON:
	    if (!have_targets)
		_usage();
	    dprintf(server_fd, CP_ON CP_EOL, targstr);
	    res = _process_response();
	    _expect(CP_PROMPT);
	    break;
	case CMD_OFF:
	    if (!have_targets)
		_usage();
	    dprintf(server_fd, CP_OFF CP_EOL, targstr);
	    res = _process_response();
	    _expect(CP_PROMPT);
	    break;
	case CMD_RESET:
	    if (!have_targets)
		_usage();
	    dprintf(server_fd, CP_RESET CP_EOL, targstr);
	    res = _process_response();
	    _expect(CP_PROMPT);
	    break;
	case CMD_CYCLE:
	    if (!have_targets)
		_usage();
	    dprintf(server_fd, CP_CYCLE CP_EOL, targstr);
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
    printf("\
  -c --cycle     Power cycle targets   -r --reset    Reset targets\n\
  -1 --on        Power on targets      -0 --off      Power off targets\n\
  -q --query     List on/off/unknown   -l --list     List available targets\n\
  -h --help      Display this help\n");
    exit(1);
}

/*
 * Set up connection to server and get to the command prompt.
 */
static void _connect_to_server(void)
{
    struct addrinfo hints, *addrinfo;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    Getaddrinfo(SERVER_HOSTNAME, SERVER_PORT, &hints, &addrinfo);

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

    while (size > 1) { /* leave room for terminating null */
	if (Read(server_fd, p, 1) <= 0)
	    err_exit(TRUE, "lost connection with server\n");
	if (*p == '\r')
	    continue;
	if (*p == '\n')
	    break;
	size--; p++;
    }
    *p = '\0';
    if (strlen(buf) > 4)
	printf("%s\n", buf + 4);
    else
	err_exit(FALSE, "unexpected response from server\n");
    num = strtol(buf, (char **)NULL, 10);
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
