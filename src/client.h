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

#include "buffer.h"

#define NO_PORT           (-1)

typedef enum { CLI_IDLE, CLI_READING, CLI_WRITING, CLI_DONE } Client_Status;
typedef struct {
    bool 		loggedin;	/* has the client logged in? */
    Client_Status 	read_status;
    Client_Status 	write_status;
    int 		seq;		/* command sequence (per client) */
    int 		fd;		/* file desriptor for  the socket */
    String 		ip;		/* IP address of the client's host */
    unsigned short int 	port;		/* Port of client connection */
    String 		host;		/* host name of client host */
    Buffer 		to;		/* out buffer */
    Buffer 		from;		/* in buffer */
    MAGIC;
} Client;

void cli_reply(Action *act);

void cli_init(void);
void cli_fini(void);

void cli_listen(void);

bool cli_exists(Client *cli);

void cli_process_select(fd_set *rset, fd_set *wset, bool over_time);
void cli_prepfor_select(fd_set *rset, fd_set *wset, int *maxfd);

#endif				/* CLIENT_H */
