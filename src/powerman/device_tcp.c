/************************************************************\
 * Copyright (C) 2001 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

/*
 * Implement connect/disconnect/preprocess methods for tcp/telnet devices.
 *
 * NOTE: we always do telnet when we are doing tcp.  If there ever is a telnet
 * device that requires some proactive option negotiation, or a non-telnet
 * device that manages to confuse the telnet state machine, we could easily
 * implement a 'notelnet' flag.  Right now it seems innocuous to leave the
 * telnet machine running.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <assert.h>
#define TELOPTS
#define TELCMDS
#include <arpa/telnet.h>
#include <sys/time.h>

#include "list.h"
#include "hostlist.h"
#include "cbuf.h"
#include "parse_util.h"
#include "xmalloc.h"
#include "xpoll.h"
#include "pluglist.h"
#include "arglist.h"
#include "xregex.h"
#include "device_private.h"
#include "error.h"
#include "debug.h"
#include "device_tcp.h"
#include "xpty.h"

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;                  /* socklen_t is uint32_t in Posix.1g */
#endif /* !HAVE_SOCKLEN_T */

typedef enum { TELNET_NONE, TELNET_CMD, TELNET_OPT } TelnetState;
typedef struct {
    char *host;
    char *port;
    TelnetState tstate;         /* state of telnet processing */
    unsigned char tcmd;         /* buffered telnet command */
    bool quiet;                 /* don't report idle timeout messages */
    struct addrinfo *addrs;
    struct addrinfo *cur;
} TcpDev;

static void _telnet_init(Device *dev);
static void _telnet_preprocess(Device * dev);

static void _parse_options(TcpDev *tcp, char *flags)
{
    char *tmp = xstrdup(flags);
    char *opt = strtok(tmp, ",");

    while (opt) {
        if (strcmp(opt, "quiet") == 0)
            tcp->quiet = true;
        else
            err_exit(false, "bad device option: %s\n", opt);
        opt = strtok(NULL, ",");
    }
    xfree(tmp);
}

void *tcp_create(char *host, char *port, char *flags)
{
    TcpDev *tcp = (TcpDev *)xmalloc(sizeof(TcpDev));
    struct addrinfo hints;
    int error;

    tcp->host = xstrdup(host);
    tcp->port = xstrdup(port);
    tcp->tstate = TELNET_NONE;
    tcp->tcmd = 0;
    tcp->quiet = false;
    if (flags)
        _parse_options(tcp, flags);

    /* Store a list of possible addresses/families to connect to
     * in the tcp->addrs linked list.
     */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((error = getaddrinfo(tcp->host, tcp->port, &hints, &tcp->addrs)) != 0)
        err_exit(false, "getaddrinfo %s:%s: %s", tcp->host, tcp->port,
                                                 gai_strerror(error));
    if (tcp->addrs == NULL)
        err_exit(false, "no addresses for server %s:%s", tcp->host, tcp->port);
    tcp->cur = tcp->addrs;

    return (void *)tcp;
}

void tcp_destroy(void *data)
{
    TcpDev *tcp = (TcpDev *)data;

    if (tcp->host)
        xfree(tcp->host);
    if (tcp->port)
        xfree(tcp->port);
    if (tcp->addrs)
        freeaddrinfo(tcp->addrs);

    xfree(tcp);
}

/* After a successful connect, perform some management on
 * the connection.  Returns true on success, false on error.
 */
static bool tcp_finish_connect_one(Device *dev)
{
    int rc;
    int error = 0;
    socklen_t len = sizeof(error);

    rc = getsockopt(dev->fd, SOL_SOCKET, SO_ERROR, &error, &len);
    /*
     *  If an error occurred, Berkeley-derived implementations
     *    return 0 with the pending error in 'error'.  But Solaris
     *    returns -1 with the pending error in 'errno'.  -dun
     */
    if (rc < 0)
        error = errno;
    if (! error) {
        dev->connect_state = DEV_CONNECTED;
        dev->stat_successful_connects++;
        _telnet_init(dev);
        return true;
    }
    return false;
}

/* Obtain a socket for the specified address and attempt to connect it.
 * Return true on completion or connection in progress, false on error.
 */
static bool tcp_connect_one(Device *dev, struct addrinfo *addr)
{
    TcpDev *tcp = (TcpDev *)dev->data;
    int opt;

    assert(tcp->cur != NULL);

    if ((dev->fd = socket(addr->ai_family, addr->ai_socktype, 0)) < 0)
        return false;
    opt = 1;
    if (setsockopt(dev->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(dev->fd);
        return false;
    }
    nonblock_set(dev->fd);

    if (connect(dev->fd, addr->ai_addr, addr->ai_addrlen) >= 0)
        return tcp_finish_connect_one(dev);
    else if (errno == EINPROGRESS)
        return true;

    close(dev->fd);
    return false;
}

/*
 * Continue TCP connect when fd unblocks.
 * Return false on error, which triggers timed retry of tcp_connect().
 * Return true if connected or still connecting (on the next address).
 */
bool tcp_finish_connect(Device * dev)
{
    TcpDev *tcp;

    assert(dev->connect_state == DEV_CONNECTING);

    tcp = (TcpDev *)dev->data;

    if (!tcp_finish_connect_one(dev)) {
        tcp->cur = tcp->cur->ai_next;
        while (tcp->cur && !tcp_connect_one(dev, tcp->cur))
            tcp->cur = tcp->cur->ai_next;
        if (tcp->cur == NULL)
            dev->connect_state = DEV_NOT_CONNECTED;
    }
    switch(dev->connect_state) {
        case DEV_NOT_CONNECTED:
            err(false, "tcp_finish_connect(%s): connection refused", dev->name);
            break;
        case DEV_CONNECTED:
            if (!tcp->quiet)
                err(false, "tcp_finish_connect(%s): connected", dev->name);
            break;
        case DEV_CONNECTING:
            if (!tcp->quiet)
                err(false, "tcp_finish_connect(%s): connecting", dev->name);
            break;
    }
    return (dev->connect_state != DEV_NOT_CONNECTED);
}

/*
 * Initiate a non-blocking TCP connect.  tcp_finish_connect() will try to
 * finish the job when the main poll() loop unblocks again, unless we
 * finish here.
 */
bool tcp_connect(Device * dev)
{
    TcpDev *tcp;

    assert(dev->connect_state == DEV_NOT_CONNECTED);
    assert(dev->fd == NO_FD);

    tcp = (TcpDev *)dev->data;

    dev->connect_state = DEV_CONNECTING;
    while (tcp->cur && !tcp_connect_one(dev, tcp->cur))
        tcp->cur = tcp->cur->ai_next;
    if (tcp->cur == NULL)
        dev->connect_state = DEV_NOT_CONNECTED;

    switch(dev->connect_state) {
        case DEV_NOT_CONNECTED:
            err(false, "tcp_connect(%s): connection refused", dev->name);
            break;
        case DEV_CONNECTED:
            if (!tcp->quiet)
                err(false, "tcp_connect(%s): connected", dev->name);
            break;
        case DEV_CONNECTING:
            if (!tcp->quiet)
                err(false, "tcp_connect(%s): connecting", dev->name);
            break;
    }

    return (dev->connect_state == DEV_CONNECTED);
}

/*
 * Close the socket associated with this device.
 */
void tcp_disconnect(Device * dev)
{
    TcpDev *tcp;

    assert(dev->connect_state == DEV_CONNECTING
           || dev->connect_state == DEV_CONNECTED);
    tcp = (TcpDev *)dev->data;

    dbg(DBG_DEVICE, "tcp_disconnect: %s on fd %d", dev->name, dev->fd);

    /* close socket if open */
    if (dev->fd >= 0) {
        if (close(dev->fd) < 0)
            err(true, "tcp_disconnect: %s close fd %d", dev->name, dev->fd);
        dev->fd = NO_FD;
    }

    if (!tcp->quiet)
        err(false, "tcp_disconnect(%s): disconnected", dev->name);
}

void tcp_preprocess(Device *dev)
{
    _telnet_preprocess(dev);
}

static void _telnet_sendopt(Device *dev, int cmd, int opt)
{
    unsigned char str[] = { IAC, cmd, opt };
    int n;

    dbg(DBG_TELNET, "%s: _telnet_sendopt: %s %s", dev->name,
            TELCMD_OK(cmd) ? TELCMD(cmd) : "<unknown>",
            TELOPT_OK(opt) ? TELOPT(opt) : "<unknown>");

    n = cbuf_write(dev->to, str, 3, NULL);
    if (n < 3)
        err((n < 0), "_telnet_sendopt: cbuf_write returned %d", n);
}

#if 0
static void _telnet_sendcmd(Device *dev, unsigned char cmd)
{
    unsigned char str[] = { IAC, cmd };
    int n;

    dbg(DBG_TELNET, "%s: _telnet_sendcmd: %s", dev->name,
            TELCMD_OK(cmd) ? TELCMD(cmd) : "<unknown>");

    n = cbuf_write(dev->to, str, 2, NULL);
    if (n < 2)
        err((n < 0), "_telnet_sendcmd: cbuf_write returned %d", n);
}
#endif

static void _telnet_recvcmd(Device *dev, int cmd)
{
    dbg(DBG_TELNET, "%s: _telnet_recvcmd: %s", dev->name,
            TELCMD_OK(cmd) ?  TELCMD(cmd) : "<unknown>");
}


static void _telnet_recvopt(Device *dev, int cmd, int opt)
{
    TcpDev *tcp = (TcpDev *)dev->data;

    dbg(DBG_TELNET, "%s: _telnet_recvopt: %s %s", dev->name,
            TELCMD_OK(cmd) ? TELCMD(cmd) : "<unknown>",
            TELOPT_OK(opt) ? TELOPT(opt) : "<unknown>");
    switch (cmd) {
    case DO:
        switch (opt) {
            case TELOPT_SGA:            /* rfc 858 - suppress go ahead*/
            case TELOPT_TM:             /* rfc 860 - timing mark */
                _telnet_sendopt(dev, WILL, opt);
                break;
            case TELOPT_TTYPE:          /* rfc 1091 - terminal type */
            case TELOPT_NAWS:           /* rfc 1073 - window size */
            /* next three added for gnat powerman/634 - jg */
            case TELOPT_NEW_ENVIRON:    /* environment variables */
            case TELOPT_XDISPLOC:       /* X display location */
            case TELOPT_TSPEED:         /* terminal speed */
            /* next two added for newer baytechs - jg */
            case TELOPT_ECHO:           /* echo */
            case TELOPT_LFLOW:          /* remote flow control */
            /* next one added for cyclades ts - jg */
            case TELOPT_BINARY:         /* 8-bit data path */
                _telnet_sendopt(dev, WONT, opt);
                break;
            default:
                if (!tcp->quiet)
                    err(0, "%s: _telnet_recvopt: ignoring %s %s", dev->name,
                            TELCMD_OK(cmd) ? TELCMD(cmd) : "<unknown>",
                            TELOPT_OK(opt) ? TELOPT(opt) : "<unknown>");
                break;
        }
        break;
    default:
        break;
    }
}

/*
 * Called just after a connect so we can perform any option requests.
 */
static void _telnet_init(Device * dev)
{
    TcpDev *tcp = (TcpDev *)dev->data;

    tcp->tstate = TELNET_NONE;
    tcp->tcmd = 0;
#if 0
    _telnet_sendopt(dev, DONT, TELOPT_NAWS);
    _telnet_sendopt(dev, DONT, TELOPT_TTYPE);
    _telnet_sendopt(dev, DONT, TELOPT_ECHO);
#endif
}

/*
 * Telnet state machine.  This is called when new data has arrived in the
 * input buffer.  We get to look first to process any telnet escapes.
 * Except for a little bit of state stored in the dev->u.tcp union,
 * we do all the processing now.
 */
static void _telnet_preprocess(Device * dev)
{
    static unsigned char peek[MAX_DEV_BUF];
    static unsigned char device[MAX_DEV_BUF];
    TcpDev *tcp = (TcpDev *)dev->data;
    int len, i, k;

    len = cbuf_peek(dev->from, peek, MAX_DEV_BUF);
    for (i = 0, k = 0; i < len; i++) {
        switch (tcp->tstate) {
        case TELNET_NONE:
            if (peek[i] == IAC)
                tcp->tstate = TELNET_CMD;
            else
                device[k++] = peek[i];
            break;
        case TELNET_CMD:
            switch (peek[i]) {
            case IAC:           /* escaped IAC */
                device[k++] = peek[i];
                tcp->tstate = TELNET_NONE;
                break;
            case DONT:          /* option commands - one more byte coming */
            case DO:
            case WILL:
            case WONT:
                tcp->tcmd = peek[i];
                tcp->tstate = TELNET_OPT;
                break;
            default:            /* single char commands - process immediately */
                _telnet_recvcmd(dev, peek[i]);
                tcp->tstate = TELNET_NONE;
                break;
            }
            break;
        case TELNET_OPT:        /* option char - process stored command */
            _telnet_recvopt(dev, tcp->tcmd, peek[i]);
            tcp->tstate = TELNET_NONE;
            break;
        }
    }
    /* rewrite buffers if anything changed */
    if (k < len) {
        int n;

        n = cbuf_drop(dev->from, len);
        if (n < len)
            err((n < 0), "_telnet_preprocess: cbuf_drop returned %d", n);
        n = cbuf_write(dev->from, device, k, NULL);
        if (n < k)
            err((n < 0), "_telnet_preprocess: cbuf_write returned %d", n);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
