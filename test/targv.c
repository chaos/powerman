/*****************************************************************************
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "argv.h"

int
main(int argc, char *argv[])
{
	char **av;

	av = argv_create("foo bar baz", "");
	assert(argv_length(av) == 3);
	av = argv_append(av, "bonk");
	assert(argv_length(av) == 4);
	assert(strcmp(av[0], "foo") == 0);
	assert(strcmp(av[1], "bar") == 0);
	assert(strcmp(av[2], "baz") == 0);
	assert(strcmp(av[3], "bonk") == 0);
	assert(av[4] == NULL);
	argv_destroy(av);

	av = argv_create("a,b:c d", ",:");
	assert(argv_length(av) == 4);
	assert(strcmp(av[0], "a") == 0);
	assert(strcmp(av[1], "b") == 0);
	assert(strcmp(av[2], "c") == 0);
	assert(strcmp(av[3], "d") == 0);
	assert(av[4] == NULL);
	argv_destroy(av);

	exit(0);
}
