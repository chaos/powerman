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
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
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

#define CMD_MAGIC 0x5565aafd
typedef struct {
    int magic;
    char *fmt;
    char **argv;
    char *sendstr;
} cmd_t;

#if WITH_GENDERS
static void _push_genders_hosts(hostlist_t targets, char *s);
#endif
static int  _connect_to_server_tcp(char *host, char *port, int retries);
static int  _connect_to_server_pipe(char *server_path, char *config_path,
                                    bool short_circuit_delays);
static void _usage(void);
static void _license(void);
static void _version(void);
static int  _process_line(int fd);
static void _expect(int fd, char *str);
static int  _process_response(int fd);
static void _process_version(int fd);
static void _cmd_create(List cl, char *fmt, char *arg, bool prepend);
static void _cmd_destroy(cmd_t *cp);
static void _cmd_append(cmd_t *cp, char *arg);
static void _cmd_prepare(cmd_t *cp, bool genders);
static int  _cmd_execute(cmd_t *cp, int fd);
static void _cmd_print(cmd_t *cp);

static char *prog;

#define OPTIONS "0:1:c:r:f:u:B:blQ:qP:tD:dTxgh:S:C:YVLZIR:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"on",          required_argument,  0, '1'},
    {"off",         required_argument,  0, '0'},
    {"cycle",       required_argument,  0, 'c'},
    {"reset",       required_argument,  0, 'r'},
    {"flash",       required_argument,  0, 'f'},
    {"unflash",     required_argument,  0, 'u'},
    {"beacon",      required_argument,  0, 'B'},
    {"beacon-all",  no_argument,        0, 'b'},
    {"list",        no_argument,        0, 'l'},
    {"query",       required_argument,  0, 'Q'},
    {"query-all",   no_argument,        0, 'q'},
    {"temp",        required_argument,  0, 'P'},
    {"temp-all",    no_argument,        0, 't'},
    {"device",      required_argument,  0, 'D'},
    {"device-all",  no_argument,        0, 'd'},
    {"telemetry",   no_argument,        0, 'T'},
    {"exprange",    no_argument,        0, 'x'},
    {"genders",     no_argument,        0, 'g'},
    {"server-host", required_argument,  0, 'h'},
    {"server-path", required_argument,  0, 'S'},
    {"config-path", required_argument,  0, 'C'},
    {"short-circuit-delays", no_argument,  0, 'Y'},
    {"version",     no_argument,        0, 'V'},
    {"license",     no_argument,        0, 'L'},
    {"dump-cmds",   no_argument,        0, 'Z'},
    {"ignore-errs", no_argument,        0, 'I'},
    {"retry-connect", required_argument, 0, 'R'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int main(int argc, char **argv)
{
    int c;
    int res = 0;
    int server_fd;
    char *p, *port = DFLT_PORT;
    char *host = DFLT_HOSTNAME;
    bool genders = false;
    bool dumpcmds = false;
    bool ignore_errs = false;
    char *server_path = NULL;
    char *config_path = NULL;
    unsigned long retry_connect = 0;
    List commands;  /* list-o-cmd_t's */
    ListIterator itr;
    cmd_t *cp;
    bool short_circuit_delays = false;

    prog = basename(argv[0]);
    err_init(prog);
    commands = list_create((ListDelF)_cmd_destroy);

    /* Parse options.
     */
    opterr = 0;
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
        case '1':              /* --on */
            _cmd_create(commands, CP_ON, optarg, false);
            break;
        case '0':              /* --off */
            _cmd_create(commands, CP_OFF, optarg, false);
            break;
        case 'c':              /* --cycle */
            _cmd_create(commands, CP_CYCLE, optarg, false);
            break;
        case 'r':              /* --reset */
            _cmd_create(commands, CP_RESET, optarg, false);
            break;
        case 'l':              /* --list */
            _cmd_create(commands, CP_NODES, NULL, false);
            break;
        case 'Q':              /* --query */
            _cmd_create(commands, CP_STATUS, optarg, false);
            break;
        case 'q':              /* --query-all */
            _cmd_create(commands, CP_STATUS_ALL, NULL, false);
            break;
        case 'f':              /* --flash */
            _cmd_create(commands, CP_BEACON_ON, optarg, false);
            break;
        case 'u':              /* --unflash */
            _cmd_create(commands, CP_BEACON_OFF, optarg, false);
            break;
        case 'B':              /* --beacon */
            _cmd_create(commands, CP_BEACON, optarg, false);
            break;
        case 'b':              /* --beacon-all */;
            _cmd_create(commands, CP_BEACON_ALL, NULL, false);
            break;
        case 'P':              /* --temp */
            _cmd_create(commands, CP_TEMP, optarg, false);
            break;
        case 't':              /* --temp-all */
            _cmd_create(commands, CP_TEMP_ALL, NULL, false);
            break;
        case 'D':              /* --device */
            _cmd_create(commands, CP_DEVICE, optarg, false);
            break;
        case 'd':              /* --device-all */
            _cmd_create(commands, CP_DEVICE_ALL, NULL, false);
            break;
        case 'Y':              /* --short-circuit-delays */
            short_circuit_delays = true;
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
            _cmd_create(commands, CP_TELEMETRY, NULL, true);
            break;
        case 'x':              /* --exprange */
            _cmd_create(commands, CP_EXPRANGE, NULL, true);
            break;
        case 'g':              /* --genders */
#if WITH_GENDERS
            genders = true;
#else
            err_exit(false, "not configured with genders support");
#endif
            break;
        case 'S':              /* --server-path */
            server_path = optarg;
            break;
        case 'C':              /* --config-path */
            config_path = optarg;
            break;
        case 'Z':              /* --dump-cmds */
            dumpcmds = true;
            break;
        case 'I':              /* --ignore-errs */
            ignore_errs = true;
            break;
        case 'R':              /* --retry-connect=N (sleep 1s after fail) */
            errno = 0;
            retry_connect = strtoul (optarg, NULL, 10);
            if (errno != 0 || retry_connect < 1)
                err_exit(false, "invalid --retry-connect argument");
            break;
        default:
            _usage();
            /*NOTREACHED*/
            break;
        }
    }
    if (list_is_empty(commands))
        _usage();
    if (short_circuit_delays && !server_path)
        _usage();

    /* For backwards compat with powerman 2.0 and earlier,
     * additional arguments are more targets for last command.
     */
    if (optind < argc) {
        cmd_t *last = NULL;

        itr = list_iterator_create(commands);
        while ((cp = list_next(itr)))
            last = cp;
        list_iterator_destroy(itr);
        if (last == NULL)
            _usage();
        while (optind < argc) {
            _cmd_append(last, argv[optind]);
            optind++;
        }
    }

    /* Prepare commands for processing.
     */
    itr = list_iterator_create(commands);
    while ((cp = list_next(itr)))
        _cmd_prepare(cp, genders);
    list_iterator_destroy(itr);

    /* Dump commands and exit if requested.
     */
    if (dumpcmds) {
        itr = list_iterator_create(commands);
        while ((cp = list_next(itr)))
            _cmd_print(cp);
        list_iterator_destroy(itr);
        exit(0);
    }

    /* Establish connection to server and start protocol.
     */
    if (server_path)
        server_fd = _connect_to_server_pipe(server_path, config_path,
                                            short_circuit_delays);
    else
        server_fd = _connect_to_server_tcp(host, port, retry_connect);
    _process_version(server_fd);
    _expect(server_fd, CP_PROMPT);

    /* Execute the commands.
     */
    itr = list_iterator_create(commands);
    while ((cp = list_next(itr))) {
        res = _cmd_execute(cp, server_fd);
        if (ignore_errs)
            res = 0;
        if (res != 0)
            break;
    }
    list_iterator_destroy(itr);
    list_destroy(commands);

    /* Disconnect from server.
     */
    hfdprintf(server_fd, "%s%s", CP_QUIT, CP_EOL);
    _expect(server_fd, CP_RSP_QUIT);

    exit(res);
}

/* Display powerman usage and exit.
 */
static void _usage(void)
{
    printf("Usage: %s [action] [targets]\n", prog);
    printf("  -1,--on targets      Power on targets\n");
    printf("  -0,--off targets     Power off targets\n");
    printf("  -c,--cycle targets   Power cycle targets\n");
    printf("  -q,--query-all       Query power state of all targets\n");
    printf("  -Q,--query targets   Query power state of specific targets\n");
#if WITH_GENDERS
    printf("  -g,--genders         Interpret targets as attributes\n");
#endif
    printf("  -h,--server-host host[:port]  Connect to remote server\n");
    printf("  -x,--exprange        Expand host ranges in query response\n");
    printf("  -T,--telemtery       Show device conversation for debugging\n");
    printf("  -l,--list            List available targets\n");
    printf ("The following are not implemented by all devices:\n");
    printf("  -r,--reset targets   Assert hardware reset\n");
    printf("  -f,--flash targets   Turn beacon on\n");
    printf("  -u,--unflash targets Turn beacon off\n");
    printf("  -b,--beacon targets  Query beacon status\n");
    printf("  -P,--temp targets    Query temperature\n");
    exit(1);
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
    exit(1);
}

/* Display powerman version and exit.
 */
static void _version(void)
{
    printf("%s\n", PACKAGE_VERSION);
    exit(1);
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

static void _cmd_create(List cl, char *fmt, char *arg, bool prepend)
{
    cmd_t *cp = (cmd_t *)xmalloc(sizeof(cmd_t));

    cp->magic = CMD_MAGIC;
    cp->fmt = fmt;
    cp->argv = NULL;
    cp->sendstr = NULL;
    if (arg)
        cp->argv = argv_create(arg, "");
    if (prepend)
        list_prepend(cl, cp);
    else
        list_append(cl, cp);
}

static void _cmd_destroy(cmd_t *cp)
{
    assert(cp->magic == CMD_MAGIC);

    cp->magic = 0;
    if (cp->sendstr)
        xfree(cp->sendstr);
    if (cp->argv)
        argv_destroy(cp->argv);
    xfree(cp);
}

static void _cmd_append(cmd_t *cp, char *arg)
{
    assert(cp->magic == CMD_MAGIC);
    if (cp->argv == NULL) {
        if (!strcmp(cp->fmt, CP_STATUS_ALL)) {
            cp->fmt = CP_STATUS;
            cp->argv = argv_create(arg, "");
        } else if (!strcmp(cp->fmt, CP_BEACON_ALL)) {
            cp->fmt = CP_BEACON;
            cp->argv = argv_create(arg, "");
        } else if (!strcmp(cp->fmt, CP_DEVICE_ALL)) {
            cp->fmt = CP_DEVICE;
            cp->argv = argv_create(arg, "");
        } else if (!strcmp(cp->fmt, CP_TEMP_ALL)) {
            cp->fmt = CP_TEMP;
            cp->argv = argv_create(arg, "");
        } else
            err_exit(false, "option takes no arguments");
    } else
        cp->argv = argv_append(cp->argv, arg);
}

static void _cmd_prepare(cmd_t *cp, bool genders)
{
    char tmpstr[CP_LINEMAX];
    hostlist_t hl;
    int i;

    assert(cp->magic == CMD_MAGIC);
    assert(cp->sendstr == NULL);

    tmpstr[0] = '\0';
    if (cp->argv) {
        hl = hostlist_create(NULL);
        for (i = 0; i < argv_length(cp->argv); i++) {
            if (genders) {
#if WITH_GENDERS
                _push_genders_hosts(hl, cp->argv[i]);
#endif
            } else {
                if (hostlist_push(hl, cp->argv[i]) == 0)
                    err_exit(false, "hostlist error");
            }
        }
        if (hostlist_ranged_string(hl, sizeof(tmpstr), tmpstr) == -1)
            err_exit(false, "hostlist error");
        hostlist_destroy(hl);
    }
    cp->sendstr = hsprintf(cp->fmt, tmpstr);
}

static int _cmd_execute(cmd_t *cp, int fd)
{
    int res;

    assert(cp->magic == CMD_MAGIC);
    assert(cp->sendstr != NULL);

    hfdprintf(fd, "%s%s", cp->sendstr, CP_EOL);
    res = _process_response(fd);
    _expect(fd, CP_PROMPT);

    return res;
}

static void _cmd_print(cmd_t *cp)
{
    assert(cp->magic == CMD_MAGIC);
    assert(cp->sendstr != NULL);

    printf("%s%s", cp->sendstr, CP_EOL);
}

static int _connect_to_server_pipe(char *server_path, char *config_path,
                                   bool short_circuit_delays)
{
    int saved_stderr;
    char cmd[128];
    char **argv;
    pid_t pid;
    int fd;

    saved_stderr = dup(STDERR_FILENO);
    if (saved_stderr < 0)
        err_exit(true, "dup stderr");
    snprintf(cmd, sizeof(cmd), "powermand -sf -c %s", config_path);
    argv = argv_create(cmd, "");
    if (short_circuit_delays)
        argv = argv_append(argv, "-Y");
    pid = xforkpty(&fd, NULL, 0);
    switch (pid) {
        case -1:
            err_exit(true, "forkpty error");
        case 0: /* child */
            if (dup2(saved_stderr, STDERR_FILENO) < 0)
                err_exit(true, "dup2 stderr");
            close(saved_stderr);
            xcfmakeraw(STDIN_FILENO);
            execv(server_path, argv);
            err_exit(true, "exec %s", server_path);
        default: /* parent */
            close(saved_stderr);
            break;
    }
    argv_destroy(argv);
    return fd;
}

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
