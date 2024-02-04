/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
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
#include <string.h>
#include <getopt.h>
#if HAVE_GENDERS_H
#include <genders.h>
#endif
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <libgen.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include "powerman.h"
#include "xmalloc.h"
#include "xread.h"
#include "error.h"
#include "hostlist.h"
#include "client_proto.h"
#include "debug.h"
#include "argv.h"
#include "xpty.h"
#include "hprintf.h"
#include "argv.h"
#include "list.h"

#if WITH_GENDERS
static void _push_genders_hosts(hostlist_t targets, char *s);
#endif
static int  _connect_to_server_tcp(char *host, char *port, int retries);
static void _usage(void);
static void _license(void);
static void _version(void);
static int  _process_line(int fd);
static void _expect(int fd, char *str);
static int  _process_response(int fd);
static void _process_version(int fd);
static void _set_command (const char **command, const char *value);

static char *prog;

#define OPTIONS "01crfubqtldTxgh:VLR:H"
static const struct option longopts[] = {
    // command
    {"on",          no_argument,        0, '1'},
    {"off",         no_argument,        0, '0'},
    {"cycle",       no_argument,        0, 'c'},
    {"reset",       no_argument,        0, 'r'},
    {"flash",       no_argument,        0, 'f'},
    {"unflash",     no_argument,        0, 'u'},
    {"beacon",      no_argument,        0, 'b'},
    {"query",       no_argument,        0, 'q'},
    {"temp",        no_argument,        0, 't'},
    {"list",        no_argument,        0, 'l'},
    {"device",      no_argument,        0, 'd'},
    // options
    {"telemetry",   no_argument,        0, 'T'},
    {"exprange",    no_argument,        0, 'x'},
    {"genders",     no_argument,        0, 'g'},
    {"server-host", required_argument,  0, 'h'},
    {"version",     no_argument,        0, 'V'},
    {"license",     no_argument,        0, 'L'},
    {"retry-connect", required_argument, 0, 'R'},
    {"help",        no_argument,        0, 'H'},
    {0, 0, 0, 0},
};

int main(int argc, char **argv)
{
    int c;
    int res = 0;
    int server_fd;
    char *p, *port = DFLT_PORT;
    char *host = DFLT_HOSTNAME;
    bool genders = false;
    unsigned long retry_connect = 0;
    const char *command = NULL;
    bool telemetry = false;
    bool exprange = false;
    hostlist_t targets;
    bool targets_required = false;

    prog = basename(argv[0]);
    err_init(prog);
    targets = hostlist_create(NULL);

    /* Parse options.
     */
    opterr = 0;
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != -1) {
        switch (c) {
        case '1':              /* --on */
            _set_command(&command, CP_ON);
            targets_required = true;
            break;
        case '0':              /* --off */
            _set_command(&command, CP_OFF);
            targets_required = true;
            break;
        case 'c':              /* --cycle */
            _set_command(&command, CP_CYCLE);
            targets_required = true;
            break;
        case 'r':              /* --reset */
            _set_command(&command, CP_RESET);
            targets_required = true;
            break;
        case 'l':              /* --list */
            _set_command(&command, CP_NODES);
            break;
        case 'q':              /* --query */
            _set_command(&command, CP_STATUS);
            break;
        case 'f':              /* --flash */
            _set_command(&command, CP_BEACON_ON);
            targets_required = true;
            break;
        case 'u':              /* --unflash */
            _set_command(&command, CP_BEACON_OFF);
            targets_required = true;
            break;
        case 'b':              /* --beacon */
            _set_command(&command, CP_BEACON);
            break;
        case 't':              /* --temp */
            _set_command(&command, CP_TEMP);
            break;
        case 'd':              /* --device */
            _set_command(&command, CP_DEVICE);
            break;
        case 'h':              /* --server-host host[:port] */
            if ((p = strchr(optarg, ':'))) {
                *p++ = '\0';
                port = p;
            }
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
            telemetry = true;
            break;
        case 'x':              /* --exprange */
            exprange = true;
            break;
        case 'g':              /* --genders */
#if WITH_GENDERS
            genders = true;
#else
            err_exit(false, "not configured with genders support");
#endif
            break;
        case 'R':              /* --retry-connect=N (sleep 100ms after fail) */
            errno = 0;
            retry_connect = strtoul (optarg, NULL, 10);
            if (errno != 0 || retry_connect < 1)
                err_exit(false, "invalid --retry-connect argument");
            break;
        case 'H':              /* --help */
            _usage();
            /*NOTREACHED*/
        default:
            err_exit(false,
                     "Unknown option.  Try '--help' for more information.");
            /*NOTREACHED*/
            break;
        }
    }
    if (!command)
        err_exit(false, "No action was specified.");
    /* Combine free arguments into target hostlist.
     * If --genders was selected, convert to hosts.
     * Then convert back to a single hostlist-compressed argument.
     * If there were no arguments, the result is the empty string.
     */
    while (optind < argc) {
        if (!genders) {
            if (!hostlist_push(targets, argv[optind++]))
                err_exit(false, "hostlist error");
        }
#if WITH_GENDERS
        else
            _push_genders_hosts(targets, argv[optind++]);
#endif
    }
    char argument[CP_LINEMAX];
    if (hostlist_ranged_string(targets, sizeof(argument), argument) == -1)
        err_exit(false, "hostlist error");

    /* Now that free arguments have been processed,
     * Fail if 'command' doesn't accept an argument (%s) but there is one,
     * or if the command requires an argument and there isn't one.
     */
    if (!strstr(command, "%s") && strlen (argument) > 0) // e.g. --nodes
        err_exit(false, "Command does not accept targets");
    if (targets_required && strlen (argument) == 0)
        err_exit(false, "Command requires targets");

    /* Establish connection to server and start protocol.
     */
    server_fd = _connect_to_server_tcp(host, port, retry_connect);
    _process_version(server_fd);
    _expect(server_fd, CP_PROMPT);

    /* First send commands that set options.
     */
    if (telemetry) {
        hfdprintf(server_fd, "%s%s" , CP_TELEMETRY, CP_EOL);
        res = _process_response(server_fd);
        _expect(server_fd, CP_PROMPT);
        if (res != 0)
            goto done;
    }
    if (exprange) {
        hfdprintf(server_fd, "%s%s", CP_EXPRANGE, CP_EOL);
        res = _process_response(server_fd);
        _expect(server_fd, CP_PROMPT);
        if (res != 0)
            goto done;
    }
    /* Send the main command.
     * Use 'command' as the format string if it contains '%s' for an argument.
     */
    if (strstr (command, "%s")) {
        hfdprintf(server_fd, command, argument);
        hfdprintf(server_fd, CP_EOL);
    }
    else
        hfdprintf(server_fd, "%s%s", command, CP_EOL);
    res = _process_response(server_fd);
    _expect(server_fd, CP_PROMPT);

    /* Disconnect from server.
     */
done:
    hfdprintf(server_fd, "%s%s", CP_QUIT, CP_EOL);
    _expect(server_fd, CP_RSP_QUIT);

    exit(res);
}

/* Display powerman usage and exit.
 */
static void _usage(void)
{
    printf("Usage: %s [options] [command] [targets]\n", prog);
    printf(
"Commands:\n"
"  -1,--on              Power on targets\n"
"  -0,--off             Power off targets\n"
"  -c,--cycle           Power cycle targets\n"
"  -q,--query           Query power state on optional targets\n"
"  -r,--reset           Assert hardware reset on targets\n"
"  -f,--flash           Turn beacon on on targets\n"
"  -u,--unflash         Turn beacon off on targets\n"
"  -b,--beacon          Query beacon status on optional targets\n"
"  -P,--temp            Query temperature on optional targets\n"
"  -l,--list            List available targets\n"
"  -d,--device          Show status of devices that control optional targets\n"
"Options:\n"
#if WITH_GENDERS
"  -g,--genders         Interpret targets as attributes\n"
#endif
"  -h,--server-host host[:port]\n"
"                       Connect to remote server\n"
"  -x,--exprange        Expand host ranges in query response\n"
"  -V,--version         Show powerman version\n"
"  -L,--license         Show powerman license\n"
"  -T,--telemtery       Show device conversation for debugging\n"
"  -R,--retry-connect=N Retry connect to server up to N times\n"
    );
    exit(0);
}

/* Display powerman license and exit.
 */
static void _license(void)
{
    printf(
 "Copyright (C) 2001 The Regents of the University of California.\n"
 "(c.f. DISCLAIMER, COPYING)\n"
 "\n"
 "This file is part of Powerman, a remote power management program.\n"
 "For details, see https://github.com/chaos/powerman.\n"
 "\n"
 "SPDX-License-Identifier: GPL-2.0-or-later\n");
    exit(0);
}

/* Display powerman version and exit.
 */
static void _version(void)
{
    printf("%s\n", PACKAGE_VERSION);
    exit(0);
}

static void _set_command (const char **command, const char *value)
{
    if (*command)
        err_exit(false, "Only one action may be specified");
    *command = value;
}

#if WITH_GENDERS
static void _push_genders_hosts(hostlist_t targets, char *s)
{
    genders_t g;
    char **nodes;
    int len, n, i;

    if (strlen(s) == 0)
        return;
    if (!(g = genders_handle_create()))
        err_exit(false, "genders_handle_create failed");
    if (genders_load_data(g, NULL) < 0)
        err_exit(false, "genders_load_data: %s", genders_errormsg(g));
    if ((len = genders_nodelist_create(g, &nodes)) < 0)
        err_exit(false, "genders_nodelist_create: %s", genders_errormsg(g));
    if ((n = genders_query(g, nodes, len, s)) < 0)
        err_exit(false, "genders_query: %s", genders_errormsg(g));
    genders_handle_destroy(g);
    if (n == 0)
        err_exit(false, "genders expression did not match any nodes");
    for (i = 0; i < n; i++) {
        if (!hostlist_push(targets, nodes[i]))
            err_exit(false, "hostlist error");
    }
}
#endif

static int _connect_any(struct addrinfo *addr)
{
    struct addrinfo *r;
    int fd = -1;
    for (r = addr; r != NULL; r = r->ai_next) {
        if ((fd = socket(r->ai_family, r->ai_socktype, 0)) < 0)
            continue;
        if (connect(fd, r->ai_addr, r->ai_addrlen) < 0) {
            close(fd);
            fd = -1;
            continue;
        }
        break; /* success! */
    }
    return fd;
}

static int _connect_to_server_tcp(char *host, char *port, int retries)
{
    int error, fd = -1;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((error = getaddrinfo(host, port, &hints, &res)) != 0)
        err_exit(false, "getaddrinfo %s:%s: %s", host, port,
                       gai_strerror(error));
    if (res == NULL)
        err_exit(false, "no addresses for server %s:%s", host, port);

    while ((fd = _connect_any(res)) < 0 && retries-- > 0)
        usleep(100000); // 100ms
    if (fd < 0)
        err_exit(false, "could not connect to address %s:%s", host, port);

    freeaddrinfo(res);
    return fd;
}

/* Return true if response should be suppressed.
 */
static bool _suppress(int num)
{
    if (strtol(CP_RSP_QRY_COMPLETE, NULL, 10) == num)
        return true;
    if (strtol(CP_RSP_TELEMETRY, NULL, 10) == num)
        return true;
    if (strtol(CP_RSP_EXPRANGE, NULL, 10) == num)
        return true;
    return false;
}

/* Get a line from the socket and display on stdout.
 * Return the numerical portion of the response.
 */
static int _process_line(int fd)
{
    char *buf = xreadstr(fd);
    long int num;

    num = strtol(buf, NULL, 10);
    if (num == LONG_MIN || num == LONG_MAX)
        num = -1;
    if (strlen(buf) > 4) {
        if (!_suppress(num))
            printf("%s\n", buf + 4);
    } else
        err_exit(false, "unexpected response from server");
    xfree(buf);
    return num;
}

/* Read version and warn if it doesn't match the client's.
 */
static void _process_version(int fd)
{
    char *buf = xreadstr(fd);
    char *vers = xmalloc (strlen(buf)+1);

    if (sscanf(buf, CP_VERSION, vers) != 1)
        err_exit(false, "unexpected response from server");
    if (strcmp(vers, PACKAGE_VERSION) != 0)
        err(false, "warning: server version (%s) != client (%s)",
                vers, PACKAGE_VERSION);
    xfree(buf);
    xfree(vers);
}

static int _process_response(int fd)
{
    int num;

    do {
        num = _process_line(fd);
    } while (!CP_IS_ALLDONE(num));
    return (CP_IS_FAILURE(num) ? num : 0);
}

/* Read strlen(str) bytes from file descriptor and exit if
 * it doesn't match 'str'.
 */
static void _expect(int fd, char *str)
{
    int len = strlen(str);
    char *buf = xmalloc (len + 1);
    char *p = buf;
    int res;

    do {
        res = xread(fd, p, len);
        if (res < 0)
           err_exit(true, "lost connection with server");
        p += res;
        *p = '\0';
        len -= res;
    } while (strcmp(str, buf) != 0 && len > 0);

    /* Shouldn't happen.  We are not handling the general case of the server
     * returning the wrong response.  Read() loop above may hang in that case.
     */
    if (strcmp(str, buf) != 0)
        err_exit(false, "unexpected response from server");
    xfree (buf);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
