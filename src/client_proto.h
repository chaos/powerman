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
 * 5. server sends response (see note under Responses below)
 * If not quit, goto 3
 */

#define CP_LINEMAX	256		/* max request/response line length */

#define CP_EOL		"\r\n"		/* line terminator */
#define CP_PROMPT 	"powerman> "	/* prompt */
#define CP_VERSION 	"001 powerman protocol version 2.0" CP_EOL

/* 
 * Requests
 */
#define CP_HELP		"help"
#define CP_QUIT		"quit"
#define CP_RESET	"reset %s"
#define CP_CYCLE	"cycle %s"
#define CP_ON		"on %s"
#define CP_OFF		"off %s"
#define CP_NODES	"nodes"
#define CP_STATUS	"status %s"
#define CP_STATUS_ALL	"status"
#define CP_SOFT		"soft %s"
#define CP_SOFT_ALL	"soft"
#define CP_TEMP		"temp %s"
#define CP_TEMP_ALL	"temp"
#define CP_BEACON	"beacon %s"
#define CP_BEACON_ALL	"beacon"
#define CP_BEACON_ON	"beacon_on %s"
#define CP_BEACON_OFF	"beacon_off %s"

/* 
 * Responses - 
 * 1XX's are successful (indicates end of response)
 * 2XX's are failure (indicates end of response)
 * 3XX's are informational
 * Responses can be multi-line.  Client knows response is complete when
 * it reads a 1XX or 2XX line.
 */
#define CP_IS_SUCCESS(i)	((i) >= 100 && (i) < 200)
#define CP_IS_FAILURE(i)	((i) >= 200 && (i) < 300)
#define CP_IS_ALLDONE(i)	((i) >= 100 && (i) < 300)

/* success */
#define CP_RSP_SUCCESS	"100 Success"			CP_EOL	/* generic */
#define CP_RSP_NODES	"101 %s" 			CP_EOL	/* hostlist */
#define CP_RSP_STATUS	\
	"302 on:      %s"				CP_EOL	\
       	"302 off:     %s" 				CP_EOL	\
	"102 unknown: %s" 				CP_EOL	/* hostlists */
#define CP_RSP_QUIT	"103 Goodbye"			CP_EOL
#define CP_RSP_HELP 	\
	"304 nodes            - query node list" 	CP_EOL	\
	"304 status [<nodes>] - query power status"	CP_EOL	\
	"304 on <nodes>       - power on" 		CP_EOL	\
	"304 off <nodes>      - power off"		CP_EOL	\
	"304 cycle <nodes>    - power cycle"	CP_EOL	\
	"304 reset <nodes>    - hardware reset (if available)" CP_EOL \
	"304 soft [<nodes>]   - query soft power status (if available)" CP_EOL \
	"304 temp [<nodes>]   - query temperature (if available)" CP_EOL \
	"304 beacon [<nodes>] - query beacon status (if available)" CP_EOL \
	"304 help             - display help"	CP_EOL	\
	"104 quit             - logout"		CP_EOL
#define CP_RSP_COMPLETE	"105 Command completed successfully" CP_EOL
#define CP_RSP_RAW	"305 %s: %s"			CP_EOL
#define CP_RSP_RAW_DONE "106 Query complete"		CP_EOL

/* failure */
#define CP_ERR_FAILURE	"200 Failure"			CP_EOL /* generic */
#define CP_ERR_UNKNOWN	"201 Unknown command"		CP_EOL
#define CP_ERR_PARSE	"202 Parse error"		CP_EOL
#define CP_ERR_TOOLONG	"203 Command too long"		CP_EOL
#define CP_ERR_INTERNAL	"204 Internal error"		CP_EOL
#define CP_ERR_HLRANGE	"205 Too many hosts in range"	CP_EOL
#define CP_ERR_HLINVAL	"206 Invalid hostlist range"	CP_EOL
#define CP_ERR_HLUNK	"207 Hostlist error"		CP_EOL
#define CP_ERR_NOSUCHNODES "208 No such nodes: %s"	CP_EOL
#define CP_ERR_COMPLETE	"209 Command completed with errors" CP_EOL
#define CP_ERR_TIMEOUT	"310 Device %s timed out" 	CP_EOL
#define CP_ERR_CLIBUSY	"211 Command in progress" 	CP_EOL
#define CP_ERR_NOACTION	"213 Command causes no action" 	CP_EOL
#define CP_ERR_UNIMPL	"214 Command cannot be handled by power control device(s)" CP_EOL

#endif /* CLIENT_PROTO_H */
