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

#ifndef LISTENER_H
#define LISTENER_H

#define LISTEN_BACKLOG    5


/*
 *  The listener port is sp[ecified in the config file.  The file 
 * descriptor here is the one the listener ends up listeneing on. (duh)
 * The "read" field is always TRUE in this implementation, but is used
 * to govern whether to include the listener fd in the select().
 */
struct listener_struct {
	int port;              
	int fd;
	bool read;
	MAGIC;
};

/* listener.c extern prototypes */
Listener *make_Listener(void);
void init_Listener(Listener *listener);
void handle_Listener(Globals *g);
void free_Listener(Listener *listener);


/* TCP-wrappers support */
int hosts_ctl(char *daemon, char *client_name, char *client_addr, 
		char *client_user);

#endif /* LISTENER_H */
