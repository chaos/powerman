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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <arpa/telnet.h>


typedef int bool;
#define TRUE  1 /* bool values */
#define FALSE 0

#define NO_FD           (-1)
#define	MAXFD	         64

#define NDUMP 1  /* Don't produce global data structure dump routines */
/* #define NDEBUG 1 Don't produce debug code */
#ifndef NDEBUG
/* Use debugging macros */

#define ASSERT(f)    \
        if(f)        \
        {}           \
        else         \
        exit_error("Assertion failed: %s, line %u", __FILE__, __LINE__)
#define MAGIC            int magic
#define INIT_MAGIC(x)    (x)->magic = (MAGIC_VAL)
#define CHECK_MAGIC(x)   ASSERT((x)->magic == MAGIC_VAL)
#define CLEAR_MAGIC(x)   (x)->magic = (0)

#define MAGIC_VAL          0xdeadbee0
#else
/* Don't use debugging macros */
#define ASSERT(f)
#define MAGIC            
#define INIT_MAGIC(x, y) 
#define CHECK_MAGIC(x)
#define CLEAR_MAGIC(x)   
#endif

#define MAX(x, y) (((x) > (y))? (x) : (y))
#define MIN(x, y) (((x) < (y))? (x) : (y))
#define foreach(x, y) for (x = (y)->next; x != y; x = (x)->next)
#define forcount(x, y) for (x = 0; x < y; (x)++)


typedef void Sigfunc(int);


typedef struct string_struct String;
typedef struct buffer_struct Buffer;
typedef enum client_status_enum Client_Status;
typedef struct client_struct Client;
typedef struct action_struct Action;
typedef enum snd_exp_enum Script_El_T;
typedef struct send_struct Send_T;
typedef struct expect_struct Expect_T;
typedef struct delay_struct Delay_T;
typedef union s_e_type_union S_E_U;
typedef struct snd_exp_struct Script_El;
typedef struct spec_element_struct Spec_El;
typedef struct specification_struct Spec;
typedef enum stateval_enum State_Val;
typedef struct interpretation_struct Interpretation;
typedef struct node_struct Node;
typedef struct cluster_struct Cluster;
typedef enum string_mode_enum String_Mode;
typedef enum device_enum Dev_Type;
typedef struct protocol_struct Protocol;
typedef struct listener_struct Listener;
typedef struct plug_struct Plug;
typedef struct tty_dev_struct TTY_Dev;
typedef struct tcp_dev_struct TCP_Dev;
typedef struct snmp_dev_struct SNMP_Dev;
typedef struct telnet_dev_struct telnet_Dev;
typedef struct pmd_dev_struct PMD_Dev;
typedef union dev_type_union Dev_U;
typedef struct dev_struct Device;
typedef enum server_status_enum Server_Status;
typedef struct log_struct Log;
typedef struct globals_struct Globals;

#include "list.h"
#include "exit_error.h"
#include "pm_string.h"
#include "buffer.h"
#include "log.h"
#include "wrappers.h"


 


