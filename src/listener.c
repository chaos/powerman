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


#include <arpa/inet.h>
#include <syslog.h>
#include <tcpd.h>
#include <fcntl.h>
#include <assert.h>

#include "powerman.h"
#include "wrappers.h"
#include "list.h"
#include "config.h"
#include "powermand.h"
#include "server.h"
#include "buffer.h"
#include "exit_error.h"
#include "log.h"
#include "pm_string.h"
#include "listener.h"

int allow_severity = LOG_INFO;     /* logging level for accepted reqs */
int deny_severity  = LOG_WARNING;  /* logging level for rejected reqs */

/*
 *   Constructor
 *
 * Produces:  Listener
 */
Listener *
make_Listener()
{
	Listener *listener;

	listener = (Listener *)Malloc(sizeof(Listener));
	INIT_MAGIC(listener);

	listener->port = NO_PORT;
	listener->fd   = NO_FD;
	listener->read = FALSE;
	return listener;
}

/*
 *  This is a conventional implementation of the code in Stevens.
 * The Listener already exists and on entry and on completion the
 * descriptor is waiting on new connections.
 */ 
void
init_Listener(Listener *listener)
{
	struct sockaddr_in saddr;
	int saddr_size = sizeof(struct sockaddr_in);
	int sock_opt;
	int fd_settings;

	CHECK_MAGIC(listener);
/* 
 * "All TCP servers should specify [the SO_REUSEADDR] socket option ..."
 *                                                  - Stevens, UNP p194 
 */
	listener->fd   = Socket(PF_INET, SOCK_STREAM, 0);
	listener->read = TRUE;

	sock_opt = 1;
	Setsockopt(listener->fd, SOL_SOCKET, SO_REUSEADDR, 
		   &(sock_opt), 
		   sizeof(sock_opt));
/* 
 *   A client could abort before a ready connection is accepted.  "The fix
 * for this problem is to:  1.  Always set a listening socket nonblocking
 * if we use select ..."                            - Stevens, UNP p424
 */
	fd_settings = Fcntl(listener->fd, F_GETFL, 0);
	Fcntl(listener->fd, F_SETFL, fd_settings | O_NONBLOCK);

	saddr.sin_family = AF_INET;
	saddr.sin_port   = htons(listener->port);
	saddr.sin_addr.s_addr = INADDR_ANY;
	Bind(listener->fd, &saddr, saddr_size);

	Listen(listener->fd, LISTEN_BACKLOG);
}

/*
 * Read activity has been detected on the listener socket.  A connection
 * request has been received.  The new client is commemorated with a
 * Client data structure, it gets buffers for sending and receiving, 
 * and is vetted through TCP wrappers.
 */ 
void
handle_Listener(Globals *g)
{
	Listener *listener = g->listener;
	Client *client;
	struct sockaddr_in saddr;
	int saddr_size = sizeof(struct sockaddr_in);
	int fd_settings;
	bool accepted_client = TRUE;
	char buf[MAX_BUF];
	struct hostent *hent;
	char *ip;
	char *host;
	char *fqdn;
	char *p;

	CHECK_MAGIC(listener);

	client = make_Client();
	
	client->fd = Accept(listener->fd, &saddr, &saddr_size);
	if(client->fd < 0) 
/* client died after it initiated connect and before we could accept */
	{
		free_Client(client);
		log_it(0, "Client aborted connection attempt");
		return;
	}

	/* Got the new client, now look at TCP wrappers */
	/* get client->ip */
	if ( inet_ntop(AF_INET, &saddr.sin_addr, buf, MAX_BUF) == NULL )
	{
		Close(client->fd);
		free_Client(client);
		log_it(0, "Unable to convert network address into string");
		return;
	}
	client->to = make_Buffer(client->fd);
	client->from = make_Buffer(client->fd);
	client->port = ntohs(saddr.sin_port);
	p = buf;
	while( (p - buf < MAX_BUF) && (*p != '/') ) p++;
	if( *p == '/' ) *p = '\0';
	client->ip  = make_String(buf);
	ip = get_String(client->ip);
	fqdn = ip;
	host = STRING_UNKNOWN;

	/* get client->host */
	if ( (hent = gethostbyaddr(&saddr, sizeof(buf), AF_INET)) == NULL ) 
	{
		log_it(0, "Unable to get host name from network address");
	}
	else
	{
		client->host = make_String(hent->h_name);
		host = get_String(client->host);
		fqdn = host;
	}

	if( g->TCP_wrappers == TRUE )
	{
		accepted_client = hosts_ctl(DAEMON_NAME, host, ip,
					    STRING_UNKNOWN);
		if( accepted_client == FALSE )
		{
			Close(client->fd);
			free_Client(client);
			log_it(0, "Client rejected: <%s, %d>", fqdn, client->port);
			return;
		}
	}

/*
 * We'll need to add the new fd to the list, mark it non-blocking,
 * and initiate the PM_LOG_IN sequence.
 */
	fd_settings = Fcntl(client->fd, F_GETFL, 0);
	Fcntl(client->fd, F_SETFL, fd_settings | O_NONBLOCK);
	list_append(g->clients, (void *)client);
	log_it(0, "New connection: <%s, %d> on descriptor %d", 
	       fqdn, client->port, client->fd);
	client->write_status = CLI_WRITING;
	send_Buffer(client->to, "PowerMan V1.0.0\r\npassword> ");
}




#ifndef NDUMP

/*
 *  Debug structure dump routine 
 */
void
dump_Listener(Listener *listener)
{
	fprintf(stderr, "\tListener: %0x\n", (unsigned int)listener);
	fprintf(stderr, "\t\tport: %d\n", listener->port);
	fprintf(stderr, "\t\tfd: %d\n", listener->fd);
	fprintf(stderr, "\t\tread: ");
	if(listener->read)
		fprintf(stderr, "TRUE\n");
	else
		fprintf(stderr, "FALSE\n");
	free_Listener(listener);
}

#endif

/*
 *   Destructor
 * 
 * Destroys:  Listener
 */
void
free_Listener(Listener *listener)
{
	CHECK_MAGIC(listener);
	
	CLEAR_MAGIC(listener);
	Free(listener);
}


