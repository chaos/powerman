/************************************************************\
 * Copyright (C) 2002 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#ifndef PM_CLIENT_PROTO_H
#define PM_CLIENT_PROTO_H

/*
 * Line oriented request/response protocol for powerman daemon.
 * 1. client connects
 * 2. server sends version string
 * 3. server sends prompt
 * 4. client sends command
 * 5. server sends response (see note under Responses below)
 * If not quit, goto 3
 */

#define CP_LINEMAX  8192                /* max request/response line length */
#define CP_EOL      "\r\n"              /* line terminator */
#define CP_PROMPT   "powerman> "        /* prompt */
#define CP_VERSION  "001 %s" CP_EOL

/*
 * Requests
 */
#define CP_HELP       "help"
#define CP_QUIT       "quit"
#define CP_RESET      "reset %s"
#define CP_CYCLE      "cycle %s"
#define CP_ON         "on %s"
#define CP_OFF        "off %s"
#define CP_NODES      "nodes"
#define CP_DEVICE     "device %s"
#define CP_DEVICE_ALL "device"
#define CP_STATUS     "status %s"
#define CP_STATUS_ALL "status"
#define CP_TEMP       "temp %s"
#define CP_TEMP_ALL   "temp"
#define CP_BEACON     "beacon %s"
#define CP_BEACON_ALL "beacon"
#define CP_BEACON_ON  "flash %s"
#define CP_BEACON_OFF "unflash %s"
#define CP_TELEMETRY  "telemetry"
#define CP_EXPRANGE   "exprange"

/*
 * Responses -
 * 1XX's are successes (indicates end of response)
 * 2XX's are failures (indicates end of response)
 * 3XX's are informational messages (more data coming)
 * Responses can be multi-line.  Client knows response is complete when
 * it reads a 1XX or 2XX line.
 */
#define CP_IS_SUCCESS(i) ((i) >= 100 && (i) < 200)
#define CP_IS_FAILURE(i) ((i) >= 200 && (i) < 300)
#define CP_IS_ALLDONE(i) ((i) >= 100 && (i) < 300)

/* success 1xx */
#define CP_RSP_QUIT         "101 Goodbye"                           CP_EOL
#define CP_RSP_COM_COMPLETE "102 Command completed successfully"    CP_EOL
#define CP_RSP_QRY_COMPLETE "103 Query complete"                    CP_EOL
#define CP_RSP_TELEMETRY    "104 Telemetry %s"                      CP_EOL
#define CP_RSP_EXPRANGE     "105 Hostrange expansion %s"            CP_EOL

/* failure 2xx */
#define CP_ERR_UNKNOWN      "201 Unknown command"                   CP_EOL
#define CP_ERR_PARSE        "202 Parse error"                       CP_EOL
#define CP_ERR_TOOLONG      "203 Command too long"                  CP_EOL
#define CP_ERR_INTERNAL     "204 Internal powermand error: %s::%d"  CP_EOL
#define CP_ERR_HOSTLIST     "205 Hostlist error: %s"                CP_EOL
#define CP_ERR_CLIBUSY      "208 Command in progress"               CP_EOL
#define CP_ERR_NOSUCHNODES  "209 No such nodes: %s"                 CP_EOL
#define CP_ERR_COM_COMPLETE "210 Command completed with errors"     CP_EOL
#define CP_ERR_QRY_COMPLETE "211 Query completed with errors"       CP_EOL
#define CP_ERR_UNIMPL       "213 Command cannot be handled by power control device(s)" CP_EOL

/* informational 3xx */
#define CP_INFO_HELP  \
 "301 nodes              - query node list"                         CP_EOL \
 "301 device [<nodes>]   - query power control device status"       CP_EOL \
 "301 status [<nodes>]   - query power status"                      CP_EOL \
 "301 on <nodes>         - power on"                                CP_EOL \
 "301 off <nodes>        - power off"                               CP_EOL \
 "301 cycle <nodes>      - power cycle"                             CP_EOL \
 "301 reset <nodes>      - hardware reset (if available)"           CP_EOL \
 "301 temp [<nodes>]     - query temperature (if available)"        CP_EOL \
 "301 beacon [<nodes>]   - query beacon status (if available)"      CP_EOL \
 "301 flash <nodes>      - set beacon to ON (if available)"         CP_EOL \
 "301 unflash <nodes>    - set beacon to OFF (if available)"        CP_EOL \
 "301 telemetry          - toggle telemetry display"                CP_EOL \
 "301 exprange           - toggle host range expansion"             CP_EOL \
 "301 help               - display help"                            CP_EOL \
 "301 quit               - logout"                                  CP_EOL
#define CP_INFO_STATUS \
 "302 on:      %s"                                                  CP_EOL \
 "302 off:     %s"                                                  CP_EOL \
 "302 unknown: %s"                                                  CP_EOL
#define CP_INFO_XSTATUS     "303 %s: %s"                            CP_EOL
#define CP_INFO_DEVICE  \
 "304 %s: state=%s reconnects=%-3.3d actions=%-3.3d type=%s hosts=%s" CP_EOL
#define CP_INFO_TELEMETRY   "305 %s"                                CP_EOL
#define CP_INFO_NODES       "306 %s"                                CP_EOL
#define CP_INFO_XNODES      "307 %s"                                CP_EOL
#define CP_INFO_ACTERROR    "308 %s"                                CP_EOL

#endif  /* PM_CLIENT_PROTO_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
