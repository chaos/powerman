/*****************************************************************************\
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick (garlick@llnl.gov)
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

#ifndef EXIT_ERROR_H
#define EXIT_ERROR_H

/* Initialize error module with name of program.  Calls openlog().
 */
void err_init(char *prog);

/* Initialize syslog tag for this instance of h8power.
 * Future error calls will reference the pointer passed in - do not free.
 */
void err_setname(char *name);

/* Classes of error reporting destinations */
typedef enum { ERR_NONE, ERR_FILE, ERR_SYSLOG, ERR_CBUF } errdest_type_t;

/* Redirect errors to 'dest', a destination of type 'dest_type', e.g.
 *   err_redirect(ERR_FILE, stderr) 
 *   err_redirect(ERR_SYSLOG, NULL);
 *   err_redirect(ERR_CBUF, cb);
 */
void err_redirect(errdest_type_t dest_type, void *dest);

/* Emit error message with a newline appended (see err_redirect above).  
 * If syslogging, use LOG_ERR level.
 */
void err(const char *fmt, ...);

/* Emit error message with a newline appended (see err_redirect above), 
 * then exit. If syslogging, use LOG_ERR level.
 */
void err_exit(const char *fmt, ...);

/* Emit debug message with a newline appended (see err_redirect above).
 * If syslogging, use LOG_DEBUG level.
 */
void dbg(const char *fmt, ...);

/* Hooks for cbuf.c and list.c error handling.
 */
void lsd_fatal_error(char *file, int line, char *mesg);
void *lsd_nomem_error(char *file, int line, char *mesg);

/* vprintf and printf-like functions for cbufs.
 */
void cbuf_vprintf(cbuf_t cbuf, const char *fmt, va_list ap);
void cbuf_printf(cbuf_t cbuf, const char *fmt, ...);

#endif                          /* EXIT_ERROR_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
