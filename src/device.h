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

#define DEV_NOT_CONNECTED 0
#define DEV_CONNECTED     1
#define DEV_LOGGED_IN     2
#define DEV_SENDING       4
#define DEV_EXPECTING     8



struct plug_struct {
	String *name;       /* this is how the plug is known to the device */
	regex_t name_re;
        struct node_struct *node;      /* Reference to port->node only */
                 /* happens for power control targetting so port->node */
                 /* points back to the power control plug.  If this    */
                 /* becomes a problem I may need to add soft power     */
                 /* status Port structs to the Device struct           */
};

struct tty_dev_struct {
	String *device;
};

struct tcp_dev_struct {
	String *host;
	String *service;
};

struct snmp_dev_struct {
	String *unimplemented;
};

struct telnet_dev_struct {
	String *unimplemented;
};

struct pmd_dev_struct {
	String *host;
	String *service;
};

union dev_type_union {
	TTY_Dev    ttyd;
	TCP_Dev    tcpd;
	SNMP_Dev   snmpd;
	telnet_Dev telnetd;
	PMD_Dev    pmd;
};

struct dev_struct {
	String *name;  
	Spec *spec;
/* I could probably get the next for fields directly from spec */
	String *all;
	regex_t on_re;
	regex_t off_re;
	Dev_Type type;
	Dev_U devu;
	bool loggedin;
	bool error;
	int status;
	int fd;
	List acts;  
	struct timeval time_stamp;  /* update this after each operation */
	struct timeval timeout;
	Buffer *to;
	Buffer *from;
	int num_plugs;
        List plugs; /* The names by which the plugs are known to the dev */
	Protocol *prot;
	MAGIC;
}; 


/* device.c extern prototypes */
extern void init_Device(Device *dev);
extern void initiate_nonblocking_connect(Device *dev);
extern void map_Action_to_Device(Device *dev, Action *act);
extern void handle_Device_read(Device *dev);
extern void do_Device_connect(Device *dev);
extern void process_script(Device *dev);
extern void handle_Device_write(Device *dev);
extern bool stalled_Device(Device *dev);
extern void recover_Device(Device *dev);
extern Device *make_Device();
extern int match_Device(void *dev, void *key);
#ifndef NDUMP
extern void dump_Device(Device *dev);
#endif
extern void free_Device(void *dev);
extern Plug *make_Plug(const char *name);
extern int  match_Plug(void *plug, void *key);
#ifndef NDUMP
extern void dump_Plug(Plug *plug);
#endif
extern void free_Plug(void *plug);



#endif
