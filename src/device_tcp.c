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

/*
 * Implement connect/disconnect/preprocess methods for tcp/telnet devices.
 *
 * NOTE: we always do telnet when we are doing tcp.  If there ever is a telnet
 * device that requires some proactive option negotiation, or a non-telnet 
 * device that manages to confuse the telnet state machine, we could easily
 * implement a 'notelnet' flag.  Right now it seems innocuous to leave the
 * telnet machine running.
 */

#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <assert.h>
#define TELOPTS
#define TELCMDS
#include <arpa/telnet.h>

#include "powerman.h"
#include "list.h"
#include "parse_util.h"
#include "device.h"
#include "error.h"
#include "wrappers.h"
#include "debug.h"
#include "device_tcp.h"

typedef enum { TELNET_NONE, TELNET_CMD, TELNET_OPT } TelnetState;
typedef struct {
    char *host;
    char *port;
    TelnetState tstate;         /* state of telnet processing */
    unsigned char tcmd;         /* buffered telnet command */
} TcpDev;

static void _telnet_init(Device *dev);
static void _telnet_preprocess(Device * dev);

void *tcp_create(char *host, char *port)
{
    TcpDev *tcp = (TcpDev *)Malloc(sizeof(TcpDev));

    tcp->host = Strdup(host);
    tcp->port = Strdup(port);
    tcp->tstate = TELNET_NONE;
    tcp->tcmd = 0;

    return (void *)tcp;
}

void tcp_destroy(void *data)
{
    TcpDev *tcp = (TcpDev *)data;

    if (tcp->host)
        Free(tcp->host);
    if (tcp->port)
        Free(tcp->port);
    Free(tcp);
}

/*
 *   We've supposedly reconnected, so check if we really are.
 * If not go into reconnect mode.  If this is a succeeding
 * reconnect, log that fact.  When all is well initiate the
 * logon script.
 */
bool tcp_finish_connect(Device * dev)
{
    int rc;
    int error = 0;
    int len = sizeof(err);

    assert(dev->connect_state == DEV_CONNECTING);
    rc = getsockopt(dev->fd, SOL_SOCKET, SO_ERROR, &error, &len);
    /*
     *  If an error occurred, Berkeley-derived implementations
     *    return 0 with the pending error in 'err'.  But Solaris
     *    returns -1 with the pending error in 'errno'.  -dun
     */
    if (rc < 0)
        error = errno;
    if (error) {
        err(FALSE, "tcp_finish_connect: %s: %s", dev->name, strerror(error));
    } else {
        err(FALSE, "tcp_finish_connect: %s: connected", dev->name);
        dev->connect_state = DEV_CONNECTED;
        dev->stat_successful_connects++;
        dev->retry_count = 0;
        _telnet_init(dev);
    }

    return (dev->connect_state == DEV_CONNECTED);
}

/*
 * Nonblocking TCP connect.
 * tcp_finish_connect will finish the job.
 */
bool tcp_connect(Device * dev)
{
    TcpDev *tcp;
    struct addrinfo hints, *addrinfo;
    int sock_opt;
    int fd_settings;

    assert(dev->magic == DEV_MAGIC);
    assert(dev->connect_state == DEV_NOT_CONNECTED);
    assert(dev->fd == NO_FD);

    tcp = (TcpDev *)dev->data;

    Gettimeofday(&dev->last_retry, NULL);
    dev->retry_count++;

    memset(&hints, 0, sizeof(struct addrinfo));
    addrinfo = &hints;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    Getaddrinfo(tcp->host, tcp->port, &hints, &addrinfo);

    dev->fd = Socket(addrinfo->ai_family, addrinfo->ai_socktype,
                     addrinfo->ai_protocol);

    dbg(DBG_DEVICE, "tcp_connect: %s on fd %d", dev->name, dev->fd);

    /* set up and initiate a non-blocking connect */

    sock_opt = 1;
    Setsockopt(dev->fd, SOL_SOCKET, SO_REUSEADDR,
               &(sock_opt), sizeof(sock_opt));
    fd_settings = Fcntl(dev->fd, F_GETFL, 0);
    Fcntl(dev->fd, F_SETFL, fd_settings | O_NONBLOCK);

    /* Connect - 0 = connected, -1 implies EINPROGRESS */
    dev->connect_state = DEV_CONNECTING;
    if (Connect(dev->fd, addrinfo->ai_addr, addrinfo->ai_addrlen) >= 0)
        tcp_finish_connect(dev);
    freeaddrinfo(addrinfo);

    return (dev->connect_state == DEV_CONNECTED);
}

/*
 * Close the socket associated with this device.
 */
void tcp_disconnect(Device * dev)
{
    assert(dev->connect_state == DEV_CONNECTING
           || dev->connect_state == DEV_CONNECTED);

    err(FALSE, "tcp_disconnect: %s: disconnected", dev->name);
    dbg(DBG_DEVICE, "tcp_disconnect: %s on fd %d", dev->name, dev->fd);

    /* close socket if open */
    if (dev->fd >= 0) {
        Close(dev->fd);
        dev->fd = NO_FD;
    }
}

void tcp_preprocess(Device *dev)
{
    _telnet_preprocess(dev);
}

static void _telnet_sendopt(Device *dev, unsigned char cmd, unsigned char opt)
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

static void _telnet_recvcmd(Device *dev, unsigned char cmd)
{
    dbg(DBG_TELNET, "%s: _telnet_recvcmd: %s", dev->name, 
            TELCMD_OK(cmd) ?  TELCMD(cmd) : "<unknown>");
}


static void _telnet_recvopt(Device *dev, unsigned char cmd, unsigned char opt)
{
    dbg(DBG_TELNET, "%s: _telnet_recvopt: %s %s", dev->name, 
            TELCMD_OK(cmd) ? TELCMD(cmd) : "<unknown>", 
            TELOPT_OK(opt) ? TELOPT(opt) : "<unknown>");
    switch (cmd)
    {
    case DO:
        switch (opt) {
            case TELOPT_SGA:    /* rfc 858 */
            case TELOPT_TM:     /* rfc 860 */
                _telnet_sendopt(dev, WILL, opt);
                break;
            case TELOPT_TTYPE:  /* rfc 1091 */
            case TELOPT_NAWS:   /* rfc 1073 */
                _telnet_sendopt(dev, WONT, opt);
                break;
            default:
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
    TcpDev *tcp = (TcpDev *)dev->data;
    unsigned char peek[MAX_DEV_BUF];
    unsigned char device[MAX_DEV_BUF];
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
            err((n < 0), "_divert_telnet: cbuf_drop returned %d", n);
        n = cbuf_write(dev->from, device, k, NULL);
        if (n < k)
            err((n < 0), "_divert_telnet: cbuf_write (device) returned %d", n);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
