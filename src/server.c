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

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <syslog.h>

#include "powerman.h"
#include "list.h"
#include "config.h"
#include "powermand.h"
#include "action.h"
#include "exit_error.h"
#include "wrappers.h"
#include "pm_string.h"
#include "buffer.h"
#include "server.h"
#include "util.h"

/* prototypes */
static Action *process_input(Protocol * client_prot, Cluster * cluster,
			     Client * c);
static Action *find_Client_script(Protocol * p, Cluster * cluster,
				  String expect);
static bool match_Client_template(Cluster * cluster, Action * act,
				  regex_t * re, String expect);
#ifndef NDUMP
static void dump_Client_Status(Client_Status status);
#endif

/*
 *   Select has indicated that there is material read to
 * be read on the fd associated with the Client c. 
 *   If there was in fact stuff to read then that stuff is put
 * in the "from" buffer for the client.  If a (or several) 
 * recognizable command(s) is (are) present then it is interpreted 
 * and put in a Server Action struct which is queued to
 * be processed when the server is Quiescent.  If the 
 * material is recognizably wrong then an error is returned 
 * to the client.  If there isn't a complete command (no '\n')
 * then the "from" buf keeps the data for later completion.
 * If there was nothing to read then it may be time to 
 * close the connection to the client.   
 */
void
handle_Client_read(Protocol * client_prot, Cluster * cluster,
		   List acts, Client * c)
{
    int n;
    Action *act = NULL;

    CHECK_MAGIC(c);

    n = read_Buffer(c->from);
    if ((n < 0) && (errno == EWOULDBLOCK))
	return;
    if (n <= 0) {
	/* EOF close or wait for writes to finish */
	c->read_status = CLI_DONE;
	return;
    }

    do {
/* 
 *   If there isn't a full command left in the from buffer then this 
 * returns NULL. Otherwise it will return a newly created Action
 * act, extract the next command from the buffer, set the
 * Action fields client, seq, com, num and target for later
 * processing, and enqueue the Action.  
 */
	act = process_input(client_prot, cluster, c);
	if (act != NULL)
	    list_append(acts, act);
    }
    while (act != NULL);

}


/*
 *   Some new stuff is in the "from" buf.  
 *   Look the suff over and try to build a Server Action.  
 * Returning a NULL means something is there but not enough 
 * to interpret.  If it's recognizably a bad command then
 * return a Server Action with com = PM_ERROR.  That and the
 * "PM_CHECK_LOGIN" command is responded to immediately.  The
 * others require sending the Server Action to the devices 
 * first.
 */
Action *process_input(Protocol * client_prot, Cluster * cluster,
		      Client * c)
{
    Action *act;
    String expect;

/* Using NULL means I'll just get the next '\n' terminated string */
/* but with any '\r' or '\n' stripped off */
    expect = get_line_from_Buffer(c->from);
    if (expect == NULL)
	return NULL;

    act = find_Client_script(client_prot, cluster, expect);
    free_String(expect);
    if (act != NULL) {
	act->client = c;
	act->seq = c->seq++;
    }
    return act;
}

/*
 *   Select has notified that it is willing to accept some 
 * write data.
 *   If the client is wanting to close the connection 
 * and the write buffer is clear then close the 
 * connection.
 */
void handle_Client_write(Client * c)
{
    int n;

    CHECK_MAGIC(c);

    n = write_Buffer(c->to);
    if (n < 0)
	return;			/* EWOULDBLOCK */

    if (is_empty_Buffer(c->to))
	c->write_status = CLI_IDLE;
}

/*
 *   Returning a NULL means there is not a complete unprocessed
 * command in the "from" buffer.  I.e. nothing to be done.
 * A complete command would be indicated by a '\n'.  There
 * could be more than one.
 */
Action *find_Client_script(Protocol * p, Cluster * cluster, String expect)
{
    Action *act;
    bool found_it = FALSE;
    regex_t *re;

    act = make_Action(PM_ERROR);
    /* look through the array for a match */
    while ((!found_it) && (act->com < (NUM_SCRIPTS - 1))) {
	act->com++;
	assert(p->scripts[act->com] != NULL);
	act->itr = list_iterator_create(p->scripts[act->com]);
	act->cur = list_next(act->itr);
	assert(act->cur->type == EXPECT);
	re = &(act->cur->s_or_e.expect.exp);
	found_it = match_Client_template(cluster, act, re, expect);
	list_iterator_destroy(act->itr);
	act->itr = NULL;
    }
    if (!found_it)
	act->com = PM_ERROR;
    return act;
}

/*
 *   This function tries to match the given RegEx if it does then
 * depending on which command we're talking about it either returns
 * TRUE and is doen, or it goes on to scan for an RegEx that the
 * Client sent in.  Finding one, it puts it in tthe Action's "target"
 * field.
 */
bool
match_Client_template(Cluster * cluster, Action * act, regex_t * re,
		      String expect)
{
    int n;
    int l;
    size_t nmatch = MAX_MATCH;
    regmatch_t pmatch[MAX_MATCH];
    int eflags = 0;
    char buf[MAX_BUF];
    char *str = get_String(expect);

    CHECK_MAGIC(act);

    re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
    n = Regexec(re, str, nmatch, pmatch, eflags);
    if (n != REG_NOERROR)
	return FALSE;
    if (pmatch[0].rm_so < 0)
	return FALSE;

    switch (act->com) {
    case PM_ERROR:
	assert(FALSE);
    case PM_LOG_IN:
    case PM_CHECK_LOGIN:
    case PM_LOG_OUT:
    case PM_UPDATE_PLUGS:
    case PM_UPDATE_NODES:
/* don't need target data */
	return TRUE;
    case PM_POWER_ON:
    case PM_POWER_OFF:
    case PM_POWER_CYCLE:
    case PM_RESET:
    case PM_NAMES:
	assert((pmatch[1].rm_so >= 0) && (pmatch[1].rm_so < MAX_BUF));
	assert((pmatch[1].rm_eo >= 0) && (pmatch[1].rm_eo < MAX_BUF));
	l = pmatch[1].rm_eo - pmatch[1].rm_so;
	if (l == 0)
	    return FALSE;
	assert((l > 0) && (l < MAX_BUF));
	strncpy(buf, str + pmatch[1].rm_so, l);
	buf[l] = '\0';
	act->target = make_String(buf);
	return TRUE;
    default:
	assert(FALSE);
    }
    return FALSE;		/* shouldn't ever reach this */
}


/*
 *   this function mostly will just increment the sequence number
 * and print another prompt.  If an update command is sent it
 * first sequences through the nodes send out the'r status as a
 * '0', '1', or '?'.
 */
void client_reply(Cluster * cluster, Action * act)
{
    Client *client = act->client;
    int seq = act->seq;
    Node *node;
    ListIterator node_i;
    int n;
    size_t nm = MAX_MATCH;
    regmatch_t pm[MAX_MATCH];
    int eflags = 0;
    regex_t nre;
    reg_syntax_t cflags = REG_EXTENDED;

    CHECK_MAGIC(act);
    CHECK_MAGIC(client);
    assert(client->fd != NO_FD);

    client->write_status = CLI_WRITING;
    if ((act->client->loggedin == FALSE) && (act->com != PM_LOG_IN)) {
	send_Buffer(client->to, "LOGIN FAILED\r\n%d Password> ", seq);
    } else {
	node_i = list_iterator_create(cluster->nodes);
	switch (act->com) {
	case PM_ERROR:
	    send_Buffer(client->to, "ERROR\r\n%d PowerMan> ", seq);
	    break;
	case PM_LOG_IN:
	    act->client->loggedin = TRUE;
	case PM_CHECK_LOGIN:
	    send_Buffer(client->to, "%d PowerMan> ", seq);
	    break;
	case PM_LOG_OUT:
	    break;
	case PM_UPDATE_PLUGS:
	    while ((node = list_next(node_i))) {
		if (node->p_state == ST_ON)
		    send_Buffer(client->to, "1");
		else if (node->p_state == ST_OFF)
		    send_Buffer(client->to, "0");
		else
		    send_Buffer(client->to, "?");
	    }
	    send_Buffer(client->to, "\r\n%d PowerMan> ", seq);
	    break;
	case PM_UPDATE_NODES:
	    while ((node = list_next(node_i))) {
		if (node->n_state == ST_ON)
		    send_Buffer(client->to, "1");
		else if (node->n_state == ST_OFF)
		    send_Buffer(client->to, "0");
		else
		    send_Buffer(client->to, "?");
	    }
	    send_Buffer(client->to, "\r\n%d PowerMan> ", seq);
	    break;
	case PM_POWER_ON:
	case PM_POWER_OFF:
	case PM_POWER_CYCLE:
	case PM_RESET:
	    send_Buffer(client->to, "%d PowerMan> ", seq);
	    break;
	case PM_NAMES:
	    Regcomp(&nre, get_String(act->target), cflags);
	    while ((node = list_next(node_i))) {
		n = Regexec(&nre, get_String(node->name), nm, pm, eflags);
		if ((n != REG_NOMATCH)
		    && ((pm[0].rm_eo - pm[0].rm_so) ==
			length_String(node->name)))
		    send_Buffer(client->to, "%s\r\n",
				get_String(node->name));
	    }
	    send_Buffer(client->to, "%d PowerMan> ", seq);
	    break;
	default:
	    assert(FALSE);
	}
	list_iterator_destroy(node_i);
    }
}


/*
 *   Constructor 
 *
 * Produces:  Client
 */
Client *make_Client()
{
    Client *client;

    client = (Client *) Malloc(sizeof(Client));
    INIT_MAGIC(client);
    client->loggedin = FALSE;
    client->read_status = CLI_READING;
    client->write_status = CLI_IDLE;
    client->seq = 0;
    client->fd = NO_FD;
    client->ip = NULL;
    client->port = NO_PORT;
    client->host = NULL;
    client->to = NULL;
    client->from = NULL;
    return client;
}

/*
 *   This match utility is compatible with the list API's ListFindF
 * prototype for searching a list of Client * structs.  The match
 * criterion is an identity match on their addresses.  This comes 
 * into use in the find_Action() when there is a chance an Action 
 * no longer has a client associated with it.
 */
int match_Client(Client * client, void *key)
{
    return (client == key);
}

#ifndef NDUMP

/*
 *    Debug printout of structure contents.
 */
void dump_Client(Client * client)
{
    fprintf(stderr, "\tClient:%0x\n", (unsigned int) client);
    if (client->loggedin == TRUE)
	fprintf(stderr, "\t\tLogged in.\n");
    else
	fprintf(stderr, "\t\tNot logged in.\n");
    fprintf(stderr, "\t\tread status: ");
    dump_Client_Status(client->read_status);
    fprintf(stderr, "\t\twrite status: ");
    dump_Client_Status(client->write_status);
    fprintf(stderr, "\t\tsequence number: %d\n", client->seq);
    fprintf(stderr, "\t\tfd: %d\n", client->fd);
    dump_Buffer(client->to);
    dump_Buffer(client->from);
}

/*
 *    Debug printout of structure contents.
 */
static void dump_Client_Status(Client_Status status)
{
    if (status == CLI_IDLE)
	fprintf(stderr, "CLI_IDLE\n");
    else if (status == CLI_READING)
	fprintf(stderr, "CLI_READING\n");
    else if (status == CLI_WRITING)
	fprintf(stderr, "CLI_WRITING\n");
    else if (status == CLI_DONE)
	fprintf(stderr, "CLI_DONE\n");
    else
	fprintf(stderr, "unknown\n");
}

#endif

/*
 *   Destructor
 *
 * Destroys:  Client
 */
void free_Client(Client * client)
{
    CHECK_MAGIC(client);

    if (client->fd != NO_FD) {
	Close(client->fd);
	syslog(LOG_DEBUG, "client on descriptor %d signs off",
	       ((Client *) client)->fd);
    }
    if (client->to != NULL)
	free_Buffer(client->to);
    if (client->from != NULL)
	free_Buffer(client->from);
    Free(client);
}
