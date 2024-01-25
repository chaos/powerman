/*****************************************************************************
 *  Copyright (C) 2004 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
 *  UCRL-CODE-2002-008.
 *
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see http://github.com/chaos/powerman/
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

#include "tap.h"
#include "argv.h"

int
main(int argc, char *argv[])
{
	char **av;

	plan(NO_PLAN);

	av = argv_create("foo bar baz", "");
	ok (av != NULL,
        "argv_create foo bar baz works");
    ok (argv_length(av) == 3,
        "argv_length returns 3");
	av = argv_append(av, "bonk");
    ok (argv_length(av) == 4,
        "argv_length returns 4");
    is (av[0], "foo",
        "first arg is foo");
	is (av[1], "bar",
        "second arg is bar");
    is (av[2], "baz",
        "third arg is baz");
    is (av[3], "bonk",
        "fourth arg is bonk");
    ok (av[4] == NULL,
        "vector is NULL terminated");
	argv_destroy(av);

	av = argv_create("a,b:c d", ",:");
	ok (av != NULL,
        "argv_create a,:c d with , and : separators works");
	ok (argv_length(av) == 4,
        "argv_length is 4");
	is (av[0], "a",
        "first arg is a");
	is (av[1], "b",
        "second arg is b");
	is (av[2], "c",
        "third arg is c");
	is (av[3], "d",
        "fourth arg is d");
	ok (av[4] == NULL,
        "vector is null terminated");
	argv_destroy(av);

	done_testing();
	exit(0);
}

// vi:ts=4 sw=4 expandtab
