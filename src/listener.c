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
#include "client.h"
#include "buffer.h"
#include "error.h"
#include "string.h"
#include "listener.h"


/* tcp wrappers support */
extern int hosts_ctl(char *daemon, char *client_name, char *client_addr, 
		char *client_user);

int allow_severity = LOG_INFO;		/* logging level for accepted reqs */
int deny_severity = LOG_WARNING;	/* logging level for rejected reqs */


static int listen_fd = NO_FD;

int listen_get_fd(void)
{
    return listen_fd;
}

/*
 *  This is a conventional implementation of the code in Stevens.
 * The Listener already exists and on entry and on completion the
 * descriptor is waiting on new connections.
 */
void listen_init(void)
{
    struct sockaddr_in saddr;
    int saddr_size = sizeof(struct sockaddr_in);
    int sock_opt;
    int fd_settings;
    unsigned short listen_port;

    /* 
     * "All TCP servers should specify [the SO_REUSEADDR] socket option ..."
     *                                                  - Stevens, UNP p194 
     */
    listen_fd = Socket(PF_INET, SOCK_STREAM, 0);

    sock_opt = 1;
    Setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &(sock_opt), sizeof(sock_opt));
    /* 
     *   A client could abort before a ready connection is accepted.  "The fix
     * for this problem is to:  1.  Always set a listening socket nonblocking
     * if we use select ..."                            - Stevens, UNP p424
     */
    fd_settings = Fcntl(listen_fd, F_GETFL, 0);
    Fcntl(listen_fd, F_SETFL, fd_settings | O_NONBLOCK);

    saddr.sin_family = AF_INET;
    listen_port = conf_get_listen_port();
    saddr.sin_port = htons(listen_port);
    saddr.sin_addr.s_addr = INADDR_ANY;
    Bind(listen_fd, &saddr, saddr_size);

    Listen(listen_fd, LISTEN_BACKLOG);
}

/*
 * Read activity has been detected on the listener socket.  A connection
 * request has been received.  The new client is commemorated with a
 * Client data structure, it gets buffers for sending and receiving, 
 * and is vetted through TCP wrappers.
 */
void listen_handler(void)
{
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

    client = cli_create();

    client->fd = Accept(listen_fd, &saddr, &saddr_size);
    if (client->fd < 0)
    /* client died after it initiated connect and before we could accept */
    {
	cli_destroy(client);
	syslog(LOG_ERR, "Client aborted connection attempt");
	return;
    }

    /* Got the new client, now look at TCP wrappers */
    /* get client->ip */
    if (inet_ntop(AF_INET, &saddr.sin_addr, buf, MAX_BUF) == NULL) {
	Close(client->fd);
	cli_destroy(client);
	syslog(LOG_ERR, "Unable to convert network address into string");
	return;
    }
    client->to = buf_create(client->fd, MAX_BUF, NULL, NULL);
    client->from = buf_create(client->fd, MAX_BUF, NULL, NULL);
    client->port = ntohs(saddr.sin_port);
    p = buf;
    while ((p - buf < MAX_BUF) && (*p != '/'))
	p++;
    if (*p == '/')
	*p = '\0';
    client->ip = str_create(buf);
    ip = str_get(client->ip);
    fqdn = ip;
    host = STRING_UNKNOWN;

    /* get client->host */
    if ((hent =
	 gethostbyaddr((const char *) &saddr.sin_addr,
		       sizeof(struct in_addr), AF_INET)) == NULL) {
	syslog(LOG_ERR, "Unable to get host name from network address");
    } else {
	client->host = str_create(hent->h_name);
	host = str_get(client->host);
	fqdn = host;
    }

    if (conf_get_use_tcp_wrappers() == TRUE) {
	accepted_client = hosts_ctl(DAEMON_NAME, host, ip, STRING_UNKNOWN);
	if (accepted_client == FALSE) {
	    Close(client->fd);
	    cli_destroy(client);
	    syslog(LOG_ERR, "Client rejected: <%s, %d>", fqdn,
		   client->port);
	    return;
	}
    }

    /*
     * We'll need to add the new fd to the list, mark it non-blocking,
     * and initiate the PM_LOG_IN sequence.
     */
    fd_settings = Fcntl(client->fd, F_GETFL, 0);
    Fcntl(client->fd, F_SETFL, fd_settings | O_NONBLOCK);

    client_add(client);
    syslog(LOG_DEBUG, "New connection: <%s, %d> on descriptor %d",
	   fqdn, client->port, client->fd);
    client->write_status = CLI_WRITING;
    buf_printf(client->to, "PowerMan V1.0.0\r\npassword> ");
}

void listen_destroy(void)
{
    /* XXX add tear down here */
}

/*
 * vi:softtabstop=4
 */
