/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2004-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
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

#ifndef LIBPOWERMAN_H
#define LIBPOWERMAN_H

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct pm_handle_struct         *pm_handle_t;
typedef struct pm_node_iterator_struct  *pm_node_iterator_t;

typedef enum {
    PM_UNKNOWN      = 0,
    PM_OFF          = 1,
    PM_ON           = 2,
} pm_node_state_t;

typedef enum {
    PM_ESUCCESS     = 0,    /* success */
    PM_ERRNOVALID   = 1,    /* system call failed, see system errno */
    PM_ENOADDR      = 2,    /* failed to get address info for server */
    PM_ECONNECT     = 3,    /* connect failed */
    PM_ENOMEM       = 4,    /* out of memory */
    PM_EBADHAND     = 5,    /* bad server handle */
    PM_EBADARG      = 6,    /* bad argument */
    PM_ESERVEREOF   = 7,    /* received unexpected EOF from server */
    PM_ESERVERPARSE = 8,    /* unexpected response from server */
    PM_EUNKNOWN     = 201,  /* server: unknown command (201) */
    PM_EPARSE       = 202,  /* server: parse error (202) */
    PM_ETOOLONG     = 203,  /* server: command too long (203) */
    PM_EINTERNAL    = 204,  /* server: internal error (204) */
    PM_EHOSTLIST    = 205,  /* server: hostlist error (205) */
    PM_EINPROGRESS  = 208,  /* server: command in progress (208) */
    PM_ENOSUCHNODES = 209,  /* server: no such nodes (209) */
    PM_ECOMMAND     = 210,  /* server: command completed with errors (210) */
    PM_EQUERY       = 211,  /* server: query completed with errors (211) */
    PM_EUNIMPL      = 213,  /* server: not implemented by device (213) */
} pm_err_t;

/* flags for pm_connect() */
#define PM_CONN_INET6   1   /* connect using IPv6 only */
#define PM_CONN_COPROC  2   /* unimplemented */

pm_err_t pm_connect(char *server, void *arg, pm_handle_t *pmhp, int flags);
void     pm_disconnect(pm_handle_t pmh);

pm_err_t pm_node_status(pm_handle_t pmh, char *node, pm_node_state_t *statep);
pm_err_t pm_node_on(pm_handle_t pmh, char *node);
pm_err_t pm_node_off(pm_handle_t pmh, char *node);
pm_err_t pm_node_cycle(pm_handle_t pmh, char *node);

pm_err_t pm_node_iterator_create(pm_handle_t pmh, pm_node_iterator_t *pmip);
char *   pm_node_next(pm_node_iterator_t pmi);
void     pm_node_iterator_reset(pm_node_iterator_t pmi);
void     pm_node_iterator_destroy(pm_node_iterator_t pmi);

char *   pm_strerror(pm_err_t err, char *str, int len);

#define PM_DFLT_PORT           "10101"
#define PM_DFLT_HOST           "localhost"

#ifdef __cplusplus
  }
#endif

#endif /* PM_POWERMAN_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
