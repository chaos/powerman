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

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#define TELOPTS
#define TELCMDS
#include <arpa/telnet.h>

#include "powerman.h"
#include "list.h"
#include "parse_util.h"
#include "device.h"
#include "error.h"
#include "wrappers.h"
#include "cbuf.h"
#include "hostlist.h"
#include "debug.h"
#include "client_proto.h"
#include "device_telnet.h"

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

void telnet_init(Device * dev)
{
    _telnet_sendopt(dev, DONT, TELOPT_NAWS);
    _telnet_sendopt(dev, DONT, TELOPT_TTYPE);
    _telnet_sendopt(dev, DONT, TELOPT_ECHO);
}

/*
 * Telnet state machine.
 */
void telnet_process(Device * dev)
{
    unsigned char peek[MAX_DEV_BUF];
    unsigned char device[MAX_DEV_BUF];
    int len, i, k;

    assert(dev->type == TELNET_DEV);
    assert(dev->u.tcp.from_telnet != NULL);

    len = cbuf_peek(dev->from, peek, MAX_DEV_BUF);
    for (i = 0, k = 0; i < len; i++) {
        switch (dev->u.tcp.tstate) {
        case TELNET_NONE:
            if (peek[i] == IAC)
                dev->u.tcp.tstate = TELNET_CMD;
            else
                device[k++] = peek[i];
            break;
        case TELNET_CMD:
            switch (peek[i]) {
            case IAC:           /* escaped IAC */
                device[k++] = peek[i];
                dev->u.tcp.tstate = TELNET_NONE;
                break;
            case DONT:          /* option commands - one more byte coming */
            case DO:
            case WILL:
            case WONT:
                dev->u.tcp.tcmd = peek[i];
                dev->u.tcp.tstate = TELNET_OPT;
                break;
            default:            /* single char commands - process immediately */
                _telnet_recvcmd(dev, peek[i]);
                dev->u.tcp.tstate = TELNET_NONE;
                break;
            }
            break;
        case TELNET_OPT:        /* option char - process stored command */
            _telnet_recvopt(dev, dev->u.tcp.tcmd, peek[i]);
            dev->u.tcp.tstate = TELNET_NONE;
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
