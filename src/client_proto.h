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

#ifndef CLIENT_PROTO_H
#define CLIENT_PROTO_H

/*
 * Line oriented request/response protocol for powerman daemon.
 * 1. client connects
 * 2. server sends protocol version string
 * 3. server sends prompt
 * 4. client sends command
 * 5. server sends one structured response line consisting of a numerical
 * response code and a string whose format is command-dependent.
 * 6. server may send additional unstructured response lines which client
 * should forward to stdout/stderr of the user
 * 7. server sends prompt
 * <repeat>
 */

#define CP_LINEMAX	256		/* max request/response line length */

#define CP_VERSION 	"protocol version 2.0" 	/* protocol version */
#define CP_PROMPT 	"powerman> "
#define CP_EOL		"\r\n"		/* line terminator */

/* 
 * Requests
 */
#define CP_HELP		"help"
#define CP_QUIT		"quit"
#define CP_RESET	"reset %s"		/* arg1: hostlist */
#define CP_CYCLE	"cycle %s"		/* arg1: hostlist */
#define CP_ON		"on %s"			/* arg1: hostlist */
#define CP_OFF		"off %s"		/* arg1: hostlist */
#define CP_QUERY_NODES	"query-nodes"
#define CP_QUERY_STATUS	"query-status"

/* 
 * Structured responses
 * Response codes from 100-199 are successes, 200-299 are failures.
 */
#define CP_IS_SUCCESS(i)	((i) >= 100 && (i) < 200)
#define CP_IS_FAILURE(i)	((i) >= 200 && (i) < 300)
#define CP_IS_VALID(i)		((i) >= 100 && (i) < 300)

/* success */
#define CP_RSP_SUCCESS	"100 Success"			CP_EOL	/* generic */
#define CP_RSP_NODES	"101 %s" 			CP_EOL	/* hostlist */
#define CP_RSP_STATUS	"102 %s:%s:%s"			CP_EOL	/* on:off:unk */
#define CP_RSP_QUIT	"103 Goodbye"			CP_EOL

/* failure */
#define CP_ERR_FAILURE	"200 Failure"			CP_EOL /* generic */
#define CP_ERR_UNKNOWN	"201 Unknown command"		CP_EOL
#define CP_ERR_PARSE	"202 Parse error"		CP_EOL
#define CP_ERR_TOOLONG	"203 Command too long"		CP_EOL
#define CP_ERR_INTERNAL	"204 Internal error"		CP_EOL
#define CP_ERR_HLRANGE	"205 Too many hosts in range"	CP_EOL
#define CP_ERR_HLINVAL	"206 Invalid hostlist range"	CP_EOL
#define CP_ERR_HLUNK	"207 hostlist error"		CP_EOL

#define CP_RSP_HELP 	CP_RSP_SUCCESS				\
	"query-status <nodes> - query power status"	CP_EOL	\
	"query-nodes          - query node list" 	CP_EOL	\
	"on <nodes>           - hard power on" 		CP_EOL	\
	"off <nodes>          - hard power off"		CP_EOL	\
	"cycle <nodes>        - hard power cycle"	CP_EOL	\
	"reset <nodes>        - soft power reset"	CP_EOL	\
	"help                 - display help"		CP_EOL	\
	"quit                 - logout"			CP_EOL

#endif /* CLIENT_PROTO_H */
