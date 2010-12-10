/*****************************************************************************\
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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#define _GNU_SOURCE
#include <getopt.h>

#include "hexdump.h"
#include "list.h"
#include "cbuf.h"
#include "serial.h"
#include "tcp.h"
#include "wrappers.h"
#include "h8proto.h"
#include "h8heartbeat.h"
#include "argv.h"
#include "error.h"
#include "h8power.h"
#include "h8sniff.h"
#include "config.h"

#define MIN_H8_BUF          1024
#define MAX_H8_BUF          1024*16
#define MIN_TTY_BUF         1024
#define MAX_TTY_BUF         1024*8

#ifndef MAX
#define MAX(a,b)            ((a) > (b) ? (a) : (b))
#endif

static int      debug_flag = 0;         /* debug info to tty */

static cbuf_t   rs485out, rs485in;      /* I/O buffers for RS485 */
static cbuf_t   ttyin, ttyout;          /* I/O buffers for stdin/stdout */


/* Read a command line from the tty and return it in buf.
 * If no commands are available, return a null-terminated empty string.
 */
static void _readcmd(char *buf, int maxlen)
{
    int bytes_peeked; 
    int len = 0;
    int dropped;

    buf[0] = '\0';
    bytes_peeked = cbuf_peek(ttyin, buf, maxlen);
    if (bytes_peeked <= 0) {
        if (bytes_peeked < 0)
            err_exit("_readcmd: cbuf_peek returned %d", bytes_peeked);
        return;
    }

    for (len = 0; len < bytes_peeked; len++) {
        if (buf[len] == '\n') {
            buf[len+1] = '\0';
            break;
        }
    }
    if (len == bytes_peeked)
        return;
    dropped = cbuf_drop(ttyin, ++len);
    if (dropped != len)
        dbg("warning: _readcmd: cbuf_drop returned %d (!= %d)", dropped, len);
}

static void _cmd_help(void)
{
    cbuf_printf(ttyout, 
"list          - list nodes and their H8 status\n" \
"stat [node]   - list power status for all nodes, or for specified node\n" \
"on [node]     - turn on all nodes, or specified node\n" \
"off [node]    - turn off all nodes, or specified node\n" \
"debug         - toggle debugging level\n" \
"sniff [level] - set packet sniff level\n" \
"                (0=none, 1=headers, 2=data hex dump, 3=full hex dump\n" \
"quit          - quit program\n"
            );
}

/* User has requested power on/off/stat.  
 * Parse command and send appropriate packets.
 */
static void _cmd_power(char **argv, powercmd_t cmd)
{
    h8hb_iterator_t itr;
    h8hb_t *sp;

    if (argv[1] == NULL) {          /* all nodes */
        itr = h8hb_iterator_create();
        while ((sp = h8hb_next(itr)) != NULL) {
            if (h8hb_expired(sp))
                cbuf_printf(ttyout, "%s: H8 is down\n", sp->name);
            else
                h8power_cmd(sp, cmd);
        }
        h8hb_iterator_destroy(itr);
    } else {                        /* single node */
        if ((sp = h8hb_lookup_byname(argv[1]))) {
            if (h8hb_expired(sp))
                cbuf_printf(ttyout, "%s: H8 is down\n", sp->name);
            else
                h8power_cmd(sp, cmd);
        } else
            cbuf_printf(ttyout, "%s: unknown node name\n", argv[1]);
    }
}

/* User has requested the debug mode be toggled.
 */
static void _cmd_debug(char **argv)
{
    debug_flag = !debug_flag;
    cbuf_printf(ttyout, "debugging is %s\n", debug_flag ? "on" : "off");
    err_redirect(debug_flag ? ERR_CBUF : ERR_SYSLOG, ttyout);
}

/* User has requested a change in sniff level.
 */
static void _cmd_sniff(char **argv)
{
    if (argv[1] == NULL)  {
        cbuf_printf(ttyout, "sniff requires a numerical argument\n");
    } else  {
        char *endptr;
        long val = strtol(argv[1], &endptr, 0);

        if (val == 0 && endptr == argv[1])
            cbuf_printf(ttyout, "error parsing integer value\n");
        else if ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE)
            cbuf_printf(ttyout, "integer value would cause under/overflow\n");
        else if ((val > SNIFF_ALL))
            cbuf_printf(ttyout, "value out of range - type help\n");
        else {
            h8sniff_init(ttyout, val);
        }
    }
}

static int _process_cmdline(void)
{
    static int need_prompt = 1;
    unsigned char *buf;
    int quit = 0;

    buf = Malloc(MAX_TTY_BUF);
    do {
        if (h8power_pending())
            break;
        if (need_prompt) {
            cbuf_printf(ttyout, "h8power> ");
            need_prompt = 0;
        }
        buf[0] = '\0';
        _readcmd(buf, MAX_TTY_BUF);
        if (strlen(buf) > 0) {
            char **argv = argv_create(buf, "");

            if (argv[0] != NULL) {
                if (strcmp(argv[0], "quit") == 0) {
                    quit = 1;
                } else if (strcmp(argv[0], "help") == 0) {
                    _cmd_help();
                } else if (strcmp(argv[0], "list") == 0) {
                    h8hb_display_cache(ttyout);
                } else if (strcmp(argv[0], "debug") == 0) {
                    _cmd_debug(argv);
                } else if (strcmp(argv[0], "sniff") == 0) {
                    _cmd_sniff(argv);
                } else if (strcmp(argv[0], "off") == 0) {
                    _cmd_power(argv, POWER_OFF);
                } else if (strcmp(argv[0], "on") == 0) {
                    _cmd_power(argv, POWER_ON);
                } else if (strcmp(argv[0], "stat") == 0) {
                    _cmd_power(argv, POWER_STAT);
                } else {
                    cbuf_printf(ttyout, "unknown command - type \"help\"\n");
                }
            }
            need_prompt = 1;

            argv_destroy(argv);
        }
    } while (!quit && strlen(buf) > 0);
    Free(buf);

    return !quit;
}

/*
 * Hand packets off to various modules.
 */
static void _process_protocol(void)
{
    h8pkt_t *pkt;

    while ((pkt = h8_read_packet()) != NULL) {
        if (h8_decode_packet(pkt)) {
            if (!h8hb_process_packet(pkt))
                h8power_process_packet(pkt, ttyout);
        } else {
            dbg("_process_protocol: ignoring mangled packet");
        }
        h8_destroy_packet(pkt);
    }
}

static void _handle_write(cbuf_t buf, int fd)
{
    int n;

    n = cbuf_read_to_fd(buf, fd, -1);
    if (n < 0)
        err_exit("_handle_write(%d): %m", fd);
}

static void _handle_read(cbuf_t buf, int fd)
{
    int n;
    int dropped;

    n = cbuf_write_from_fd(buf, fd, -1, &dropped);
    if (n < 0)
        err_exit("_handle_read(%d): %m", fd);
    if (n == 0)
        err_exit("_handle_read(%d): EOF", fd);
    if (dropped != 0)
        dbg("read dropped %d bytes\n", dropped);
}

static void _select_loop(int rs485fd)
{
    Pollfd_t pfd;
    short flags;
    struct timeval tmout;
    int n;

    pfd = PollfdCreate();

    timerclear(&tmout);

    while (_process_cmdline()) {

        h8power_process_pending(&tmout, ttyout);

        h8hb_process_late_heartbeats(&tmout);

        PollfdZero(pfd);

        PollfdSet(pfd, rs485fd, POLLIN);
        if (!cbuf_is_empty(rs485out))
            PollfdSet(pfd, rs485fd, POLLOUT);

        PollfdSet(pfd, STDIN_FILENO, POLLIN);
        if (!cbuf_is_empty(ttyout))
            PollfdSet(pfd, STDOUT_FILENO, POLLOUT);

        n = Poll(pfd, timerisset(&tmout) ? &tmout : NULL);
        if (n < 0)
            err_exit("Poll: %m");

        flags = PollfdRevents(pfd, rs485fd);
        if (flags & POLLERR)
            err("rs485 poll error");
        if (flags & POLLHUP)
            err("rs485 poll hangup");
        if (flags & POLLNVAL)
            err("rs485 poll fd not open");
        if ((flags & (POLLERR | POLLHUP | POLLNVAL)))
            err_exit("lost contact with RS485, giving up");
        if ((flags & POLLIN))
            _handle_read(rs485in, rs485fd);
        if ((flags & POLLOUT))
            _handle_write(rs485out, rs485fd);

        flags = PollfdRevents(pfd, STDIN_FILENO);
        if ((flags & (POLLERR | POLLHUP | POLLNVAL)))
            err_exit("stdin poll error");
        if (flags & POLLIN)
            _handle_read(ttyin, STDIN_FILENO);

        flags = PollfdRevents(pfd, STDOUT_FILENO);
        if ((flags & (POLLERR | POLLHUP | POLLNVAL)))
            err_exit("stdout poll error");
        if (flags & POLLOUT)
            _handle_write(ttyout, STDOUT_FILENO);

        _process_protocol();
    }

    PollfdDestroy(pfd);
}

#define OPT_STRING "s:i:d:a:n:"
static const struct option long_options[] = {
    {"spnames", required_argument, 0, 's'},
    {"ignore",  required_argument, 0, 'i'},
    {"device",  required_argument, 0, 'd'},
    {"address", required_argument, 0, 'a'},
    {"name",    required_argument, 0, 'n'},
    {0, 0, 0, 0}
};
static const struct option *longopts = long_options;

static void _usage(void)
{
    err_exit(
"Usage: h8power [OPTIONS]\n"
"-s --spnames hostlist       List of expected SP Names\n"
"-i --ignore  hostlist       List of SP Names to ignore\n"
"-d --device  dev            Special file or host:port for RS-485 interface\n"
"-a --address N              RS-485 address to use (defaults to 0x01)\n"
"-n --name name              set name of program for logging"
    );
}

static int _setaddr(char *str)
{
    unsigned long val = strtol(str, NULL, 0);

    if ((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE)
        return 0;
    h8_setaddr(val);
    return 1;
}

static void _create_cbufs(void)
{
    rs485out = cbuf_create(MIN_H8_BUF, MAX_H8_BUF);
    cbuf_opt_set(rs485out, CBUF_OPT_OVERWRITE, CBUF_NO_DROP);
    rs485in = cbuf_create(MIN_H8_BUF, MAX_H8_BUF);
    cbuf_opt_set(rs485in, CBUF_OPT_OVERWRITE, CBUF_NO_DROP);

    ttyout = cbuf_create(MIN_TTY_BUF, MAX_TTY_BUF);
    cbuf_opt_set(ttyout, CBUF_OPT_OVERWRITE, CBUF_NO_DROP);
    ttyin = cbuf_create(MIN_TTY_BUF, MAX_TTY_BUF);
    cbuf_opt_set(ttyin, CBUF_OPT_OVERWRITE, CBUF_NO_DROP);
}

static void _destroy_cbufs(void)
{
    /* XXX should these be flushed first? */
    cbuf_destroy(rs485out);
    cbuf_destroy(rs485in);
    cbuf_destroy(ttyout);
    cbuf_destroy(ttyin);
}

int main(int argc, char *argv[])
{
    int fd, c, longindex;
    char *device = NULL;
    char *spnames = NULL;
    char *ignore = NULL;

    err_init(argv[0]); /* initially goes to stderr */

    while ((c = getopt_long(argc, argv, OPT_STRING, longopts,
                                              &longindex)) != -1) {
        switch (c) {
            case 'n':       /* --name */
                err_setname(optarg);
                break;
            case 's':       /* --spnames */
                spnames = optarg;
                break;
            case 'i':       /* --ignore */
                ignore = optarg;
                break;
            case 'd':       /* --device */
                device = optarg;
                break;
            case 'a':       /* --address */
                if (!_setaddr(optarg))
                    err_exit("h8power: parsing address: '%s': %m", optarg);
                break;
            default:
                _usage();
                break;
        }
    }
    if (optind < argc)
        _usage();
    if (device == NULL)
        err_exit("--device option is required");


    if (strchr(device, ':')) 
        fd = tcp_init(device);
    else 
        fd = serial_init(device); /* exits on error */
    h8hb_init(spnames, ignore);
    _create_cbufs();
    h8_init(rs485in, rs485out);
    h8power_init();
    err_redirect(ERR_SYSLOG, NULL);

    _select_loop(fd);

    h8power_fini();
    _destroy_cbufs();
    h8hb_fini();
    h8_fini();

    if (strchr(device, ':'))
        tcp_fini(fd);
    else
        serial_fini(fd);
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
