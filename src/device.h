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

/* bitwise values for dev->status */
#define DEV_NOT_CONNECTED 0
#define DEV_CONNECTING    1
#define DEV_CONNECTED     2
#define DEV_LOGGED_IN     4
#define DEV_SENDING       8
#define DEV_EXPECTING     16

/*
 * Plug
 */
typedef struct {
    char      *name;	    /* how the plug is known to the device */
    regex_t	    name_re;
    Node	    *node;	    
} Plug;

/*
 * Device
 */
struct device_struct {
    char      *name;
    char      *all;
    regex_t	    on_re;
    regex_t	    off_re;
    Dev_Type	    type;
    union {
	/*TTY_Dev ttyd;*/
	struct {
	    char    *host;
	    char    *service;
	} tcpd;
	/*SNMP_Dev snmpd;*/
	/*telnet_Dev telnetd;*/
	struct {
	    char    *host;
	    char    *service;
	} pmd;
    } devu;
    bool	    loggedin;
    bool	    error;
    int		    status;
    int		    fd;
    List	    acts;
    struct timeval  time_stamp;	    /* update this after each operation */
    struct timeval  timeout;
    Buffer	    to;
    Buffer	    from;
    int		    num_plugs;
    List	    plugs;	/* list of Plugs structures */
    Protocol	    *prot;
    bool	    logit;
    MAGIC;
};
/* FIXME: data structures are circular here - typedef in powerman.h jg */


void dev_init(void);
void dev_fini(void);
void dev_add(Device *dev);
void dev_start_all(bool logit);
void dev_nb_connect(Device * dev);
void dev_handle_read(Device * dev);
void dev_connect(Device * dev);
void dev_process_script(Device * dev);
void dev_handle_write(Device * dev);
bool dev_stalled(Device * dev);
void dev_recover(Device * dev);
void dev_apply_action(Action *act);

Device *dev_create();
void dev_destroy(Device * dev);
Device *dev_findbyname(char *name);

Plug *dev_plug_create(const char *name);
int dev_plug_match(Plug * plug, void *key);
void dev_plug_destroy(Plug * plug);

void dev_prepfor_select(fd_set *rset, fd_set *wset, int *maxfd);
bool dev_process_select(fd_set *rset, fd_set *wset, bool over_time);

#endif				/* DEVICE_H */

/*
 * vi:softtabstop=4
 */
