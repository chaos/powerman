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
#include "vicebox.h"

/*
 * Pre:  none.
 * Post: none.
 */ 
int
main(int argc, char **argv)
{
/*
 * For now there are no command line parameters.  All is configured
 * in init_vb.  This will change as I add port and logging options.
 * I may eventually have the configuration come from a vicebox.conf
 * file, at which point more command line options will emerge.
 */
	Vicebox *vice;

	vice = init_vb(argc, argv);
/* We now have a socket at vice->listener->fd running in listen mode */
	while ( 1 ) 
	{
		int maxfd;
		struct timeval tv;
		FD_List *fdl;
		int n;

/* set up and call the select */
		set_FD_List(vice->fds, &(vice->rset), READ);
		FD_SET(vice->listener->fd, &(vice->rset));
		set_FD_List(vice->fds, &(vice->wset), WRITE);
		if (vice->log->write) FD_SET(vice->log->fd, &(vice->wset));
		maxfd = max_FD_List(vice->fds);
		maxfd = MAX(maxfd, vice->listener->fd);
		maxfd = MAX(maxfd, vice->log->fd);
		tv = vice->listener->time;

		n = Select(maxfd + 1, &(vice->rset), &(vice->wset), NULL, &tv);
		if (n == 0) log_it(vice, "Timed out.");
/* 
 * There is work to be done.  Is it a new connection?
 */
		if ( FD_ISSET(vice->listener->fd, &(vice->rset)) ) 
			handle_listener(vice);

/*
 * Or a log write?
 */
		if ( FD_ISSET(vice->log->fd, &(vice->wset)) )
			handle_log(vice);
/*
 * Is there reading and writing that we need to continue managing?
 * vice->fds is a doubly linked, circular list with an EMPTY_FD node
 * always present at the head, i.e. vice->fds->fd == EMPTY_FD, so 
 * avoid that one and do the rest.
 */
		for (fdl = vice->fds->next; fdl != vice->fds; fdl = fdl->next) 
		{
			if (FD_ISSET(fdl->fd, &(vice->rset)))
				handle_read(vice, fdl);
			if (FD_ISSET(fdl->fd, &(vice->wset)))
				handle_write(vice, fdl);
			if ( fdl->status == NOT_CONNECTED )
			{
				FD_List *next = fdl->next;

				close(fdl->fd);
				del_FD_List(vice->fds, fdl);
				fdl = next;
				log_it(vice, "Connection Reset by Peer");
			}
		}
	}
	return 0;
}

/*
 * Pre:  none.
 * Post: valid vicebox returned or exit with out of memory error
 */ 
Vicebox *
init_vb(int argc, char **argv)
{
/*
 * All intialization is hard coded here for now.  If I ever developed
 * command line and/or config file info that would find its way in here 
 * as well.
 */
	Vicebox *v;
	int plug, node, expect;
	int log_fd;
	time_t t;
	int flags;
	int n;
	int port = -1;

	if (argc > 1)
	  {
	    n = sscanf(argv[1], "%d", &port);
	    if( (n != 1) || (port < 1025) || (port > 60000) )
	      port = -1;
	  }
	v = (Vicebox *)Malloc(sizeof(Vicebox), "Vicebox");
	INIT_MAGIC(v, VICEBOX_MAGIC);
	sprintf(v->name, "vicebox");
/* Log file.  spit out a time stamp */ 
	sprintf(v->log_file, "/tmp/vicebox.log.%d", port);
	flags = O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK;
	log_fd = open(v->log_file, flags, S_IRUSR | S_IWUSR);
	if (log_fd < 0)
		exit_error("Problem opening log file %s", v->log_file);
	v->log = make_FD_List(log_fd);
	t = time(NULL);
	log_it(v, ctime(&t));

	v->type = RAW_TCP;
	if( port == -1 ) v->tcp_port = VICEBOX_PORT;
	else v->tcp_port = port;
	FD_ZERO(&(v->rset));
	FD_ZERO(&(v->wset));
	FD_ZERO(&(v->eset));

/* This is where the socket in listen mode is started */
	v->listener = make_listener(v->tcp_port);

/* 
 * This is initialized to an empty list.  There is one FD_List structure 
 * always present.  It has an invalid value, EMPTY_FD == -1, for its file 
 * descriptor. 
 */
	v->fds = make_FD_List(EMPTY_FD);

/* initialize the semantics for an icebox with ten computers plugged in */
	v->off_val = '0';
	v->on_val = '1';
	v->num_plugs = NUM_ICEBOX_PLUGS;
	for( plug = 0; plug < v->num_plugs; plug++)
		v->plug_states[plug] = OFF;
	v->num_nodes = NUM_ICEBOX_PLUGS;
	for( node = 0; node < v->num_nodes; node++)
		v->node_states[node] = OFF;

/* initialize the expect/send pairs for the icebox protocol */
	v->num_expects = NUM_ICEBOX_EXPECTS;
	for ( expect = 0; expect < v->num_expects; expect++) {
		init_expect( &(v->expects[expect]) );
	}
	v->longest_expect = 0;
	v->expects[LOG_IN].in_use = TRUE;
	set_string( &(v->expects[LOG_IN].send), "OK");
	set_string( &(v->expects[LOG_IN].expect), "auth password");
	v->longest_expect = v->expects[LOG_IN].expect.length;
	v->expects[CHECK_LOGIN].in_use = TRUE;
	set_string( &(v->expects[CHECK_LOGIN].send), "OK");
	set_string( &(v->expects[CHECK_LOGIN].expect), "");
	v->longest_expect = MAX(v->longest_expect,
				v->expects[CHECK_LOGIN].expect.length);
	v->expects[LOG_OUT].in_use = TRUE; 
	set_string( &(v->expects[LOG_OUT].send), "");
	set_string( &(v->expects[LOG_OUT].expect), "q");
	v->longest_expect = MAX(v->longest_expect,
				v->expects[LOG_OUT].expect.length);
	v->expects[PLUG_QUERY].in_use = TRUE;
	set_string( &(v->expects[PLUG_QUERY].send), 
"N1:%p1 N2:%p2 N3:%p3 N4:%p4 N5:%p5 N6:%p6 N7:%p7 N8:%p8 N9:%p9 N10:%p10");
	set_string( &(v->expects[PLUG_QUERY].expect), "ps %d");
	v->longest_expect = MAX(v->longest_expect,
				v->expects[PLUG_QUERY].expect.length);
	v->expects[NODE_QUERY].in_use = TRUE;
	set_string( &(v->expects[NODE_QUERY].send), 
"N1:%n1 N2:%n2 N3:%n3 N4:%n4 N5:%n5 N6:%n6 N7:%n7 N8:%n8 N9:%n9 N10:%n10");
	set_string( &(v->expects[NODE_QUERY].expect), "ns %d");
	v->longest_expect = MAX(v->longest_expect,
				v->expects[NODE_QUERY].expect.length);
	v->expects[POWER_ON].in_use = TRUE;
	set_string( &(v->expects[POWER_ON].send), "OK");
	set_string( &(v->expects[POWER_ON].expect), "ph %d");
	v->longest_expect = MAX(v->longest_expect,
				v->expects[POWER_ON].expect.length);
	v->expects[POWER_OFF].in_use = TRUE;
	set_string( &(v->expects[POWER_OFF].send), "OK");
	set_string( &(v->expects[POWER_OFF].expect), "pl %d");
	v->longest_expect = MAX(v->longest_expect,
				v->expects[POWER_OFF].expect.length);
	v->expects[POWER_CYCLE].in_use = FALSE;
	set_string( &(v->expects[POWER_CYCLE].send), "");
	set_string( &(v->expects[POWER_CYCLE].expect), "");
	v->longest_expect = MAX(v->longest_expect,
				v->expects[POWER_CYCLE].expect.length);
	v->expects[RESET].in_use = TRUE;
	set_string( &(v->expects[RESET].send), "OK");
	set_string( &(v->expects[RESET].expect), "rp %d");
	v->longest_expect = MAX(v->longest_expect,
				v->expects[RESET].expect.length);
	v->expects[EXPECT_ERROR].in_use = TRUE;
	set_string( &(v->expects[EXPECT_ERROR].send), "ERROR 0");
	set_string( &(v->expects[EXPECT_ERROR].expect), "");
	v->longest_expect = MAX(v->longest_expect,
				v->expects[EXPECT_ERROR].expect.length);
	return v;
}


/*
 * Pre:  port is the icebox tcp port 1010.
 * Post: FD_List structure in support of a socket in listen mode 
 *         for port or exit with error
 */ 
FD_List *
make_listener(int port)
{
	FD_List *listener;

	listener = make_FD_List(EMPTY_FD);
	set_read_FD_List( listener );	
	listener->time.tv_sec = TIMEOUT_SECONDS;
	listener->time.tv_usec = 0;
	listener->fd = Socket(PF_INET, SOCK_STREAM, 0);
/* 
 * "All TCP servers should specify [the SO_REUSEADDR] socket option ..."
 *                                                  - Stevens, UNP p194 
 */
	listener->sock_opt = 1;
	Setsockopt(listener->fd, SOL_SOCKET, SO_REUSEADDR, 
		   &(listener->sock_opt), 
		   sizeof(listener->sock_opt));
/* 
 *   A client could abort before a ready connection is accepted.  "The fix
 * for this problem is to:  1.  Always set a listening socket nonblocking
 * if we use select ..."                            - Stevens, UNP p424
 */
	listener->fd_settings = Fcntl(listener->fd, F_GETFL, 0);
	Fcntl(listener->fd, F_SETFL, listener->fd_settings | O_NONBLOCK);

	listener->saddr.sin_family = AF_INET;
	listener->saddr.sin_port   = htons(port);
	listener->saddr.sin_addr.s_addr = INADDR_ANY;
	Bind(listener->fd, 
	     (Saddr *)&(listener->saddr), 
	     listener->saddr_size);

	Listen(listener->fd, LISTEN_BACKLOG);
	return listener;
}

/*
 * Pre:  Valid vicebox structure with a socket in listen mode.  The
 *       indication of activity on the file descritpor may be
 *       spurious.  Accept() will exit on serious error and return
 *       a negative number if there isn't really anything there.
 * Post: If the Accept is legit then a new FD_List structure is 
 *       created and added to the list.  It is initialized as 
 *       needing to be read.  This does not put the icebox in a
 *       LOGGED_IN state.  That happens after a successfull
 *       log in.  
 */ 
void
handle_listener(Vicebox *vice)
{
	FD_List *new;
	
	log_it(vice, "In handle_listener");
	VALIDATE(vice, VICEBOX_MAGIC);
	new = make_FD_List(EMPTY_FD);
	
	new->fd = Accept(vice->listener->fd, 
			 &(new->saddr), 
			 &(new->saddr_size));
	if(new->fd < 0) 
/* client died after it initiated connect and before we could accept */
	{
		CLEAR_MAGIC(new);
		free(new);
		log_it(vice, "Aborted connection attempt");
	}
	else
	{
/*
 * We'll need to add the new fd to the list, mark it non-blocking,
 * and mark it as needing to be read.  
 */
		add_FD_List(vice->fds, new);
		new->fd_settings = Fcntl(new->fd, F_GETFL, 0);
		Fcntl(new->fd, F_SETFL, new->fd_settings | O_NONBLOCK);
		strncpy(new->to.in, "V2.2\r\n", 6);
		new->to.in += 6;
		set_write_FD_List(new);
		FD_SET(new->fd, &(vice->wset));
		log_it(vice, "New connection");
		new->status = CONNECTED;
	}
}

void
handle_log(Vicebox *vice)
{
	int n;

	VALIDATE(vice, VICEBOX_MAGIC);

	n = write(vice->log->fd, vice->log->to.out, 
		  (int) (vice->log->to.in - vice->log->to.out));
	if ( n < 0 )
	{
		if ( errno != EWOULDBLOCK )
			exit_error("Log error");
		goto handle_log_out;
	}
	vice->log->to.out += n;
	if ( vice->log->to.out == vice->log->to.in )
	{
		vice->log->to.in = vice->log->to.out 
			= vice->log->to.start;
		vice->log->write = FALSE;
	}
	handle_log_out :
		;
}


/*
 * Pre:  Valid vicebox structure and valid FD_List structure.  The 
 *       FD_List is in read mode.  The indication of activity on
 *       the file descritpor may be spurious.  
 * Post: The Read may exit if there is a serious error.  EOF will
 *       close the file descriptor and free the FD_List struct.
 *       A completed expect phrase causes the fd to go into write
 *       mode and a send phrase is put in its "to" buffer.  An 
 *       incomlete read is noted and pointers are moved in the 
 *       "from" buffer.
 *       The semantics of a valid incoming expect phrase is 
 *       interpreted and any side effects carried out (I don't
 *       yet).
 */ 
void
handle_read(Vicebox *vice, FD_List *fdl)
{
	int n;
	int exp;
	
	VALIDATE(vice, VICEBOX_MAGIC);
	VALIDATE(fdl, FD_LIST_MAGIC);
	ASSERT(fdl->fd != EMPTY_FD);
	ASSERT(fdl != vice->log);
	ASSERT((fdl->read) && !(fdl->write) && !(fdl->exception));
	n = Read(fdl->fd, fdl->from.start, MAX_BUF);
	if ( n < 0 ) 
	{
		if(errno == EWOULDBLOCK)
			log_it(vice, "False alarm, nothing to read.");
		else
			fdl->status = NOT_CONNECTED;/* ECONNRESET */
 		goto handle_read_out;
	}
	if ( n == 0 ) 
	{
/* 
 * EOF.  The send/expect alternation is strict.  When we read EOF we
 * know we are done, because we aren't set to read at all unless the 
 * prior write is complete. 
 */
		fdl->status = NOT_CONNECTED;
		goto handle_read_out;
	}
	fdl->from.in += n;
	exp = find_expect(vice, fdl, fdl->from.start, 
			  (int)(fdl->from.in - fdl->from.start));
	if (exp == EXP_NOT_FOUND)
/* 
 * This is the case where it could be legal we justy haven't seen 
 * enough yet.
 */
	{
		log_it(vice, "Partial read.");		
		goto handle_read_out;
	}
/*	log_it(vice, fdl->from.start); */
	if (exp == ILLEGAL_EXP)
	{
		if ((fdl->error_count)++ > ERROR_LIMIT)
		{
			close(fdl->fd);
			del_FD_List(vice->fds, fdl);
		}
		else
			exp_send(vice, fdl, EXPECT_ERROR);
		goto handle_read_out;
	}
	exp_send(vice, fdl, exp);
	fdl->error_count = 0;
	handle_read_out : 
		;
}

/*
 * Pre:  A valid Vicebox structure and FD_List structure.  The 
 *       FD_List is in write mode.  The indication of activity on
 *       the file descritpor may be spurious.  
 * Post: The write may exit on a serious error.  If all that was to 
 *       be written has been done then the "to" buffer is reset
 *       and the file descriptor put back in read mode.
 */ 
void
handle_write(Vicebox *vice, FD_List *fdl)
{
	int n;

	VALIDATE(vice, VICEBOX_MAGIC);
	VALIDATE(fdl, FD_LIST_MAGIC);
	ASSERT(fdl->fd != EMPTY_FD);
	ASSERT(!(fdl->read) && (fdl->write) && !(fdl->exception));

	n = write(fdl->fd, fdl->to.out, (int) (fdl->to.in - fdl->to.out));
	if ( n < 0 )
	{
		if ( errno != EWOULDBLOCK )
			exit_error("Write error");
		goto handle_write_out;
	}
	fdl->to.out += n;
	if ( fdl->to.out == fdl->to.in )
	{
		fdl->to.in = fdl->to.out 
			= fdl->to.start;
		if (fdl != vice->log) set_read_FD_List(fdl);
	}
	handle_write_out :
		;
}

/* 
 *  FD_List handling code
 */

/*
 * Pre:  "head" is the "empty" FD_List structure.
 * Post: The returned value is the largest file descriptor in the
 *       list.
 */ 
int
max_FD_List(FD_List *head)
{
	FD_List *l;
	int max;

	VALIDATE(head, FD_LIST_MAGIC);
	max = head->fd;
	for(l = head->next; l != head; l = l->next) {
		max = MAX(max, l->fd);
	}
	return max;
}


/*
 * Pre:  "head" is the "empty" FD_List item.  
 * Post: set has all the file descriptors from the list that are
 *       supposed to be included in set for the given mode.  
 */ 
void
set_FD_List(FD_List *head, fd_set *set, int mode)
{
	FD_List *l;
	bool val = FALSE;

	VALIDATE(head, FD_LIST_MAGIC);
	FD_ZERO(set);
	for(l = head->next; l != head; l = l->next) {
		switch (mode) 
		{
		case READ :
			val = l->read;
			break;
		case WRITE :
			val = l->write;
			break;
		case EXCEPTION :
			val = l->exception;
			break;
		default :
			ASSERT(FALSE);
		}
		if ( val ) FD_SET(l->fd, set);
	}
}

/*
 * Pre:  none.
 * Post: A valid FD_List structure is produced or the routine exits
 *       due to being out of memory.  The structure is linked only
 *       to itself, i.e. it is a doubly linked circular list of 
 *       exactly one element.
 */ 
FD_List *
make_FD_List(int fd)
{
	FD_List *new = NULL;

	new = (FD_List *)Malloc(sizeof(FD_List), "FD_List");
	INIT_MAGIC(new, FD_LIST_MAGIC);
	new->next = new;
	new->prev = new;
	new->read = FALSE;
	new->write = FALSE;
	new->exception = FALSE;
	new->fd = fd;
	init_buffer( &(new->to) );
	init_buffer( &(new->from) );
	memset( &(new->saddr), 0, sizeof(Saddr));
	new->saddr_size = sizeof(Saddr);
	new->sock_opt = 0;
	new->fd_settings = 0;
	memset( &(new->time), 0, sizeof(struct timeval));	
	new->error_count = 0;
	return new;
}

#ifndef NDEBUG
/*
 * Pre:  none.
 * Post: DEBUG assertions about the structure of an FD_List.
 */ 
void
Valid_FD_List(FD_List *fdl)
{
	FD_List *item;

	ASSERT(fdl != NULL);
	ASSERT(fdl->magic == FD_LIST_MAGIC);
	if(fdl->fd == EMPTY_FD) {
		for(item = fdl->next; item != fdl; item = item->next) {
			ASSERT(item != NULL);
			ASSERT(item->magic == FD_LIST_MAGIC);
			ASSERT(item->fd != EMPTY_FD);
		}
	}
}

/*
 * Pre:  none.
 * Post: DEBUG assertions about the structure of a Vicebox.
 */ 
void
Valid_Vicebox(Vicebox *v)
{
	ASSERT(v != NULL);
	ASSERT(v->magic == VICEBOX_MAGIC);
}

#endif

/*
 * Pre:  "fdl" is valid
 * Post: it is set to read && !write && !exception
 */ 
void
set_read_FD_List(FD_List *fdl)
{
	VALIDATE(fdl, FD_LIST_MAGIC);
	fdl->read = TRUE;
	fdl->write = FALSE;
	fdl->exception = FALSE;
}

/*
 * Pre:  "fdl" is valid
 * Post: it is set to !read && write & !exception
 */ 
void
set_write_FD_List(FD_List *fdl)
{
	VALIDATE(fdl, FD_LIST_MAGIC);
	fdl->read = FALSE;
	fdl->write = TRUE;
	fdl->exception = FALSE;
}

/*
 * Pre:  "head" is the "empty" FD_List element and fdl is valid 
 * Post: fdl is the last element in the list (just before head).
 */ 
void
add_FD_List(FD_List *head, FD_List *fdl) 
{
	VALIDATE(head, FD_LIST_MAGIC);
	head->prev->next = fdl;
	fdl->prev = head->prev;
	fdl->next = head;
	head->prev = fdl;
}

/*
 * Pre:  "head" is the "empty" FD_List element
 * Post: If fd is in the list the FD_List element for it is returned
 *       otherwise NULL is returned.
 */ 
FD_List *
find_FD_List(FD_List *head, int fd)
{
	FD_List *find;

	VALIDATE(head, FD_LIST_MAGIC);
	for(find = head->next; find != head; find = find->next) {
		if (find->fd == fd) return find;
	}
	return (FD_List *) NULL;
}

/*
 * Pre:  "head" is the "empty" FD_List element, and fdl is a valid
 *       FD_List structure in the list.
 * Post: fdl is no longer in the list.
 */ 
void
del_FD_List(FD_List *head, FD_List *fdl)
{
	ASSERT(head != NULL);
	ASSERT(fdl != NULL);
#ifndef NDEBUG
	{
		FD_List *find = find_FD_List(head, fdl->fd);
		ASSERT(find == fdl);
	}
#endif
	fdl->prev->next = fdl->next;
	fdl->next->prev = fdl->prev;
	CLEAR_MAGIC(fdl);
	free(fdl);
}

/* Other support functions */

/*
 * Pre:  "s" and "cs" are valid.
 * Post: "cd" is in "s" and the length noted.
 */ 
void
set_string(String *s, const char *cs)
{
	strncpy( s->string, cs, MAX_BUF );
	s->length = strlen(s->string);
}

/*
 * Pre:  "b" is valid
 * Post: the buffer space is all zeros and the pointers all
 *       point at its start.
 */ 
void 
init_buffer(Buffer *b)
{
	INIT_MAGIC(b, BUF_MAGIC);
	memset( &(b->buf), 0, MAX_BUF + 1 );
	b->start = b->in = b->out = b->buf;
	b->end = b->buf + MAX_BUF;
}

/*
 * Pre:  "e" is valid
 * Post: the send and expect String spaces are entirely zeroed. The
 *       in_use flag is intitalized to FALSE.
 */ 
void
init_expect(Expect *e)
{
	INIT_MAGIC(e, EXPECT_MAGIC);	
	memset( &(e->send), 0, MAX_BUF + 1 );
	memset( &(e->expect), 0, MAX_BUF + 1 );
	e->in_use = FALSE;
}

/*
 * Pre:  "v" is a valid Vicebox, esp. the expects have been intialized.
 *       "str" is a '\0' terminated character array exactly len
 *       bytes long.
 * Post: If the str is definitely not in the "expect" list then the
 *       return value is ILLEGAL_EXP.  If it might be (its a prefix of
 *       a valid expect) then EXP_NOT_FOUND (yet) is returned.  If there
 *       is a match then the index of the matching expect is returned.
 *       If there is data to be interpreted that should be returned as 
 *       well, though I don't do that yet.
 */ 
int
find_expect(Vicebox *v, FD_List *fdl, char *str, int len)
/*
 *  N.B.  This will need to be modified to accomodate the 
 * vicebox patterns.  Replace strncmp() with a mode general 
 * compare function.  Any side effects might need to be carried 
 * out at this point.
 */
{
	char *pos;
	int i;
	int data;

	VALIDATE(v, VICEBOX_MAGIC);
	ASSERT(len > 0);
	if ( len > v->longest_expect + 2 ) return ILLEGAL_EXP;
	if ( (pos = (char *)memchr(str, '\n', len)) == NULL )
		return EXP_NOT_FOUND;
	*pos = '\0';
	len--;
	if (*(pos - 1) == '\r')
	{
		ASSERT(len > 0);
		pos--;
		*pos = '\0';
		len--;
	}
	if (len == 0) return CHECK_LOGIN;
	for (i = 0; i < v->num_expects; i++)
	{
		if (v->expects[i].in_use)
		{
			data = recognize_com(v, v->expects[i].expect.string, 
					     str, v->longest_expect);
			if (data != EXP_NOT_FOUND)
				return semantic_action(v, fdl, i, data);
		}
	}
	return ILLEGAL_EXP;
}

/*
 * Pre:  "v" and "fdl" are valid structures and msg a valid 
 *       expect/send index.  
 * Post: The message corresponding to msg is placed in the 
 *       fdl's "to" buffer.  Any data that needs to be put in the
 *       to buffer is also done.  
 */ 
void
exp_send(Vicebox *v, FD_List *fdl, int msg)
{
	int len;

	VALIDATE(v, VICEBOX_MAGIC);
	VALIDATE(fdl, FD_LIST_MAGIC);
	ASSERT((msg >= 0) && (msg < v->num_expects) 
	       && (v->expects[msg].in_use));
	fdl->from.in = fdl->from.out = fdl->from.start;
	len = process_com(v, fdl->to.start, 
			  v->expects[msg].send.string, 
			  v->expects[msg].send.length);
	if ( len < 0 ) exit_error("process_com failure");
	ASSERT( len < MAX_BUF  - 2);
	fdl->to.in += len;
	*(fdl->to.in++) = '\r';
	*(fdl->to.in++) = '\n';
	*(fdl->to.in)   = '\0';
	set_write_FD_List(fdl);
	FD_SET(fdl->fd, &(v->wset));
/*	log_it(v, fdl->to.start); */
}

void
log_it(Vicebox *v, char *str)
{
	int len;

	set_write_FD_List(v->log);
	FD_SET(v->log->fd, &(v->wset));
	len = strlen(str);
	strncpy(v->log->to.in, str, len);
	v->log->to.in += len;
	*(v->log->to.in) = '\n';
	(v->log->to.in)++;
	*(v->log->to.in) = '\0';
}

int
process_com(Vicebox *v, char *dest, char *src, int max)
{
	int d, s;
	int port;
	State st;

	d = 0;
	for (s = 0; s < max; s++)
	{
		if (src[s] == '\0') break;
		if (src[s] != '%')
		{
			dest[d] = src[s];
			d++;
			continue;
		}
		s++;
		ASSERT( (src[s] == 'p') || (src[s] == 'n') );
		s++;
		port = src[s] - '0';
		if ( (src[s] == '1') && (src[s+1] == '0') ) 
		{
			port = 10;
			s++;
		}
		ASSERT ( (port >= 1) && (port <= 10) ); 
		if (src[s - 1] == 'p')
			st = v->plug_states[port - 1];
		else
			st = v->node_states[port - 1];
		switch (st)
		{
		case ON :
			dest[d] = v->on_val;
			d++;
			break;
		case OFF :
			dest[d] = v->off_val;
			d++;
			break;
		case ST_UNKNOWN :
		default :
			ASSERT(FALSE);
		}
	}
	dest[d] = '\0';
	return d;
}

/*
 * pre:  v is valid, exp points to one of the expect strings, str points
 *       to a "from" buffer with an imput string, and len > 0.  
 *       The len == 0 is handled before the recognize_com function is 
 *       called.  
 * post: If str is a valid instance of the command described by exp then
 *       return EXP_FOUND (or one of '0'...'9',"*").  If it is not
 *       valid then return EXP_NOT_FOUND.  Commands that have an argument 
 *       put the value of that argument in the return variable in place 
 *       of EXP_FOUND. 
 */
int
recognize_com(Vicebox *v, char *exp, char *str, int len)
{
	int e;
	int s = 0;
	int result = EXP_FOUND;
	int data = -1;
	
	VALIDATE(v, VICEBOX_MAGIC);
/* 
 *   If we get just a "\n" or "\r\n" that is recognized as a 
 * "CHECK_LOGIN" event before "recognize_com" is acalled, so
 * arriving here with zero length is a programming error
 */
	ASSERT(len > 0);

	for (e = 0; e <= len; e++)
	{
		if ( (exp[e] == '\0') && (str[s] == '\0') )
/* success: return EXP_FOUND or data */
			return result;
		if ( (exp[e] == '\0') || (str[s] == '\0') )
/* One string ran out before the other */
			return EXP_NOT_FOUND;
		if (exp[e] == '%')
		{
/* matching against a command that takes an argument: the %d matches */
/* either '*' or '0'...'9'.  We check elsewhere if it's alegal value */
/* compared to num_plugs and num_nodes.                              */
			if (exp[++e] == 'd')
			{
				if ( (str[s] == '1') && (str[s+1] == '0') )
				{
					s++;
					data = 'A';
				}
				else
					data = str[s];
				s++;
				if ( (data == '*') || (data == 'A') ||
				     ((data >= '1') && (data <= '9')) )
					result = data;
				else
/* It's a mal-formed command, since the argument was wrong */
					return EXP_NOT_FOUND;
			}
		}
		else
			if (exp[e] != str[s++]) 
/* failed since the characters didn't match */
				return EXP_NOT_FOUND;
	}
/* ran over the match limit.  */
	return EXP_NOT_FOUND;
}


/*
 *  pre:  v is valid, exp is the index of the command in question.
 *        node is either EXP_FOUND or the value of a character '0'...'9'
 *        or '*' (for all). 
 */
int
semantic_action(Vicebox *v, FD_List *fdl, int exp, int node)
{
	int ret = exp;
	int n = node;
	State s = OFF;

	switch (exp) 
	{
	case LOG_IN :
		if (fdl->status == CONNECTED)
			fdl->status = LOGGED_IN;
		else
			ret = ILLEGAL_EXP;
		break;
	case LOG_OUT :
		if (fdl->status == LOGGED_IN)
			fdl->status = CONNECTED;
		else
			ret = ILLEGAL_EXP;
		break;
	case POWER_ON :
	case POWER_CYCLE :
		s = ON;
	case POWER_OFF :
		if ( fdl->status != LOGGED_IN ) 
		{
			ret = ILLEGAL_EXP;
			break;
		}
		if (n == '*')
		{
			for (n = 0; n < v->num_plugs; n++)
				v->plug_states[n] = s;
			for (n = 0; n < v->num_nodes; n++)
				v->node_states[n] = s;
			break;
		}
		if (n == 'A') n = 10;
		else n -= '0';
		ASSERT((1 <= n) && (n <= v->num_plugs) && (n <= v->num_nodes));
		v->plug_states[n - 1] = s;
		v->node_states[n - 1] = s;
		break;
	default :
/* In this model "reset" is a noop */
		break;
	}
	return ret;	
}
