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

#ifndef DEVICE_H
#define DEVICE_H

#include <regex.h>
#include "buffer.h"

/* bitwise values for dev->script_status */
#define DEV_LOGGED_IN	    1
#define DEV_SENDING	    2
#define DEV_EXPECTING	    4
#define DEV_DELAYING	    8

typedef enum { DEV_NOT_CONNECTED, DEV_CONNECTING, DEV_CONNECTED } ConnectStat;

/*
 * Plug
 */
typedef struct {
    char	    *name;	    /* how the plug is known to the device */
    Node	    *node;	    
} Plug;

/*
 * Device
 */
struct device_struct {
    char	    *name;	    /* name of device */
    char	    *all;	    /* string for to select all plugs */
    regex_t	    on_re;	    /* regex to match "on" in query */
    regex_t	    off_re;	    /* regex to match "off" in query */
    Dev_Type	    type;	    /* type of device e.g. TCP_DEV */
    union {			    /* type-specific device information */
	struct {
	    char    *host;
	    char    *service;
	} tcpd;
    } devu;

    ConnectStat	    connect_status;
    int		    script_status;  /* DEV_ bits reprepsenting script state */

    int		    fd;
    List	    acts;	    /* queue of Actions */

    struct timeval  time_stamp;	    /* update this after each operation */
    struct timeval  timeout;

    Buffer	    to;		    /* buffer -> device */
    Buffer	    from;	    /* buffer <- device */

    int		    num_plugs;	    /* Plug count for this device */
    List	    plugs;	    /* list of Plugs */
    Protocol	    *prot;	    /* list of expect/send scripts */

    struct timeval  last_reconnect; /* time of last reconnect attempt */
    int		    reconnect_count;/* number of reconnects attempted */
    MAGIC;
};
/* FIXME: data structures are circular here - typedef in powerman.h jg */


void dev_init(void);
void dev_fini(void);
void dev_add(Device *dev);
void dev_apply_action(Action *act);
void dev_initial_connect(void);

Device *dev_create();
void dev_destroy(Device * dev);
Device *dev_findbyname(char *name);

Plug *dev_plug_create(const char *name);
int dev_plug_match(Plug * plug, void *key);
void dev_plug_destroy(Plug * plug);

void dev_pre_select(fd_set *rset, fd_set *wset, int *maxfd);
void dev_post_select(fd_set *rset, fd_set *wset, struct timeval *tv);

#endif				/* DEVICE_H */

/*
 * vi:softtabstop=4
 */
