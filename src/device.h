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

#define DEV_NOT_CONNECTED 0
#define DEV_CONNECTING    1
#define DEV_CONNECTED     2
#define DEV_LOGGED_IN     4
#define DEV_SENDING       8
#define DEV_EXPECTING     16



typedef struct {
    String name;		/* this is how the plug is known to the device */
    regex_t name_re;
    Node *node;			/* Reference to port->node only */
    /* happens for power control targetting so port->node */
    /* points back to the power control plug.  If this    */
    /* becomes a problem I may need to add soft power     */
    /* status Port structs to the Device struct           */
} Plug;

typedef struct {
    String device;
} TTY_Dev;

typedef struct {
    String host;
    String service;
} TCP_Dev;

typedef struct {
    String unimplemented;
} SNMP_Dev;

typedef struct {
    String unimplemented;
} telnet_Dev;

typedef struct {
    String host;
    String service;
} PMD_Dev;


typedef union {
    TTY_Dev ttyd;
    TCP_Dev tcpd;
    SNMP_Dev snmpd;
    telnet_Dev telnetd;
    PMD_Dev pmd;
} Dev_U;

struct device_struct {
    String name;
    Spec *spec;
/* I could probably get the next for fields directly from spec */
    String all;
    regex_t on_re;
    regex_t off_re;
    Dev_Type type;
    Dev_U devu;
    bool loggedin;
    bool error;
    int status;
    int fd;
    List acts;
    struct timeval time_stamp;	/* update this after each operation */
    struct timeval timeout;
    Buffer to;
    Buffer from;
    int num_plugs;
    List plugs;			/* The names by which the plugs are known to the dev */
    Protocol *prot;
    bool logit;
     MAGIC;
};
/* FIXME: data structures are circular here - typedef in powerman.h jg */


/* device.c extern prototypes */
void dev_init(void);
void dev_fini(void);
void dev_start_all(bool logit);
void dev_nb_connect(Device * dev);
void dev_acttodev(Device * dev, Action * act);
void dev_handle_read(Device * dev);
void dev_connect(Device * dev);
void dev_process_script(Device * dev);
void dev_handle_write(Device * dev);
bool dev_stalled(Device * dev);
void dev_recover(Device * dev);

Device *dev_create();
int dev_match(Device * dev, void *key);
void dev_destroy(Device * dev);

Plug *dev_plug_create(const char *name);
int dev_plug_match(Plug * plug, void *key);
void dev_plug_destroy(Plug * plug);

void dev_prepfor_select(fd_set *rset, fd_set *wset, int *maxfd);
bool dev_process_select(fd_set *rset, fd_set *wset, bool over_time);

extern List powerman_devs;		/* make me private */

#endif				/* DEVICE_H */
