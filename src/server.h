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

#ifndef SERVER_H
#define SERVER_H

#define NO_PORT           (-1)


enum client_status_enum {CLI_IDLE, CLI_READING, CLI_WRITING, 
				 CLI_DONE}; 

/*
 *  Each client is represented by one of these.  The read and write
 * status move between busy and idle, except that read status goes
 * to done when there is nothing more to be read from the socket.
 * As soon as write status becomes idle and read status is done the
 * client structure is discarded.
 */
struct client_struct {
	bool loggedin;              /* has the client logged in? */
	Client_Status read_status;  
	Client_Status write_status;
	int seq;    /* Each command from the client get a sequence number */
	int fd;     /* file desriptor for  the socket */
	String *ip; /* IP address of the client's host */
	unsigned short int port;  /* Port of actual client connection */
	String *host;    /* host name of client host */
/* the two buffers for non-blocking I/O as per Stevens */
	Buffer *to;
	Buffer *from;
	MAGIC;
};

/* server.c extern prototypes */
extern void handle_Client_read(Protocol *client_prot, Cluster *cluster, 
			       List acts, Client *c);
extern void handle_Client_write(Client *c);
extern void client_reply(Cluster *cluster, Action *act);
extern Client *make_Client();
extern int match_Client(void *client, void *key);
extern void free_Client(void *client);




#endif
