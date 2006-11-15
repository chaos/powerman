/*****************************************************************************\
 *  $Id: powerman.h 689 2004-02-20 16:51:06Z garlick $
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
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

#ifndef POWERMAN_OPTIONS_H
#define POWERMAN_OPTIONS_H

#define POWERMAN_OPTION_TYPE_GET_NODES 0

/* 
 * struct powerman_option
 *
 * Describes a powerman module option
 */
struct powerman_option {
  char    option;               /* option character */
  char   *option_arg;           /* option argument description if option takes one */
  char   *option_long;          /* optional long option */
  char   *description;          /* description of option */
  int     option_type;          /* which function to call */
};

/*
 * Powerman_options_setup
 *
 * Setup options module
 *
 * Return 0 on success, -1 on error.
 */
typedef int (*Powerman_options_setup)(void);

/*
 * Powerman_options_cleanup
 *
 * Cleanup options module allocations
 *
 * Return 0 on success, -1 on error.
 */
typedef int (*Powerman_options_cleanup)(void);

/*
 * Powerman_options_process_option
 *
 * Handle the option 'c' and possibly the option argument
 * appropriately for a particular module.
 *
 * Returns 0 on success, -1 on error
 */
typedef int (*Powerman_options_process_option)(char c, char *optarg);

/*
 * Powerman_options_get_nodes
 *
 * Retrieve nodenames specified by the user
 *
 * Returns 0 on success, -1 on error.
 */
typedef int (*Powerman_options_get_nodes)(char *buf, int buflen);

/*
 * struct powerman_options_module_info
 *
 * contains options module information and operations.  Required to be
 * defined in each options module.
 */
struct powerman_options_module_info
{
  char *module_name;
  struct powerman_option *options;
  Powerman_options_setup setup;
  Powerman_options_cleanup cleanup;
  Powerman_options_process_option process_option;
  Powerman_options_get_nodes get_nodes;
};


#endif                          /* POWERMAN_OPTIONS_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
