/*****************************************************************************\
 *  $Id:$
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
    PM_ENOADDR      = 2,    /* failed to get address info for host:port */
    PM_ECONNECT     = 3,    /* connect failed */
    PM_ENOMEM       = 4,    /* out of memory */
    PM_EBADHAND     = 5,    /* bad server handle */
    PM_EBADNODE     = 6,    /* unknown node name */
    PM_EBADARG      = 7,    /* bad argument */
    PM_ETIMEOUT     = 8,    /* client timed out */
    PM_ESERVEREOF   = 9,    /* received unexpected EOF from server */
    PM_ESERVER      = 10,   /* server error */
    PM_EPARSE       = 11,   /* received unexpected response from server */
    PM_EPARTIAL     = 12,   /* command completed with errors */
    PM_EUNIMPL      = 13,   /* command not implemented by device */
} pm_err_t;

pm_err_t pm_connect(char *server, void *arg, pm_handle_t *pmhp);
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

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
