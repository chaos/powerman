/************************************************************\
 * Copyright (C) 2004 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

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
