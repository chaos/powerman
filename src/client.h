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

#ifndef CLIENT_H
#define CLIENT_H

#include "cbuf.h"
#include "hostlist.h"
#include "device.h"

#define NO_PORT           (-1)

typedef struct {
    int com;                    /* script index */
    hostlist_t hl;              /* target nodes */
    int pending;                /* count of pending device actions */
    bool error;                 /* cumulative error flag for actions */
    ArgList *arglist;           /* argument for query commands */
} Command;

typedef enum { CLI_IDLE, CLI_READING, CLI_WRITING, CLI_DONE } Client_Status;
typedef struct {
    Client_Status read_status;
    Client_Status write_status;
    int fd;                     /* file desriptor for  the socket */
    char *ip;                   /* IP address of the client's host */
    unsigned short int port;    /* Port of client connection */
    char *host;                 /* host name of client host */
    cbuf_t to;                  /* out buffer */
    cbuf_t from;                /* in buffer */
    Command *cmd;               /* command (there can be only one) */
    int client_id;              /* client identifier */
    bool verbose;               /* client wants verbose debugging info */
     MAGIC;
} Client;

void cli_init(void);
void cli_fini(void);

void cli_listen(void);

void cli_post_select(fd_set * rset, fd_set * wset);
void cli_pre_select(fd_set * rset, fd_set * wset, int *maxfd);

#endif                          /* CLIENT_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
