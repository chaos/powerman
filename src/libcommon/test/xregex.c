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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tap.h"

#include "xtypes.h"
#include "xregex.h"
#include "xmalloc.h"
#include "error.h"


/* Return true if regex [r] matches exactly [p] in [s].
 */
static bool
_matchstr(char *r, char *s, char *p)
{
	xregex_t re;
	xregex_match_t rm;
	int res;
	char *tmp;

	re = xregex_create();
	rm = xregex_match_create(2);
	xregex_compile(re, r, TRUE);
	res = xregex_exec(re, s, rm);
	if (res && p) {
		tmp = xregex_match_strdup(rm);
		if (strcmp(tmp, p) != 0)
			res = FALSE;
		xfree(tmp);
	}
	xregex_match_destroy(rm);
	xregex_destroy(re);

	return res;
}

/* Return true if regex [r] matches exactly [s] in [s].
 */
static bool
_matchstr_all(char *r, char *s)
{
	return _matchstr(r, s, s);
}

/* Return true if regex [r] matches anything in [s].
 */
static bool
_match(char *r, char *s)
{
	return _matchstr(r, s, NULL);
}

static void
_check_substr_match(void)
{
	xregex_t re;
	xregex_match_t rm;
	char *s;

	re = xregex_create();
	rm = xregex_match_create(2);

	xregex_compile(re, "foo([0-9]+)bar([0-9]+)", TRUE);
	ok (xregex_exec(re, "xxxfoo1bar2", rm) == TRUE,
        "regex with substrings matches xxxfoo1bar2");

	s = xregex_match_sub_strdup(rm, 0);
	ok (s != NULL,
        "substring 0 matched");
	is (s, "foo1bar2",
        "substring 0 is foo1bar2");
	xfree(s);

	s = xregex_match_sub_strdup(rm, 1);
	ok (s != NULL,
        "substring 1 matched");
	is (s, "1",
        "substring 1 is 1");
	xfree(s);

	s = xregex_match_sub_strdup(rm, 2);
	ok (s != NULL,
        "substring 2 matched");
	is (s, "2",
        "substring 2 is 2");
	xfree(s);

	s = xregex_match_sub_strdup(rm, 3);
	ok (s == NULL,
        "substring 4 did NOT match");

	s = xregex_match_sub_strdup(rm, -1); /* powerman actually does this! */
	ok (s == NULL,
        "substring -1 did NOT match");

	s = xregex_match_strdup(rm);
	is (s, "xxxfoo1bar2",
        "overall match is xxxfoo1bar2");

	ok (xregex_match_strlen(rm) == strlen(s),
        "xregex_match_strlen returns expected length");
	xfree(s);

	xregex_match_recycle(rm);

	ok (xregex_exec(re, "foobar2", rm) == FALSE,
        "regex does NOT match foobar2");

	s = xregex_match_sub_strdup(rm, 0);
	ok (s == NULL,
        "substring 0 did NOT match");
	s = xregex_match_sub_strdup(rm, 1);
	ok (s == NULL,
        "substring 1 did NOT match");

	xregex_match_recycle(rm);

	ok (xregex_exec(re, "xxxfoo1bar2yyy", rm) == TRUE,
        "regex does matches xxxfoo1bar2yyy");

	s = xregex_match_sub_strdup(rm, 0);
	ok (s != NULL,
        "substring 0 matched");
	is (s, "foo1bar2",
        "substring 0 is foo1bar2");
	xfree(s);

	s = xregex_match_sub_strdup(rm, 1);
	ok (s != NULL,
        "substring 1 matched");
	is (s, "1",
        "substring 1 is 1");
	xfree(s);

	s = xregex_match_sub_strdup(rm, 2);
	ok (s != NULL,
        "substring 2 matched");
	is (s, "2",
        "substring 2 is 2");
	xfree(s);

	s = xregex_match_strdup(rm);
	is (s, "xxxfoo1bar2",
        "overall match is xxxfoo1bar2");
	ok (xregex_match_strlen(rm) == strlen(s),
        "xregex_match_strlen returned expected length");
	xfree(s);

	xregex_match_destroy(rm);
	xregex_destroy(re);
}

int
main(int argc, char *argv[])
{
	char *s;

	plan(NO_PLAN);

	ok (_match("foo", "foo"),
        "regex foo matches foo");
	ok (_match("foo", "fooxxx"),
        "regex foo matches fooxxx");
	ok (_match("foo", "xxxfoo"),
        "regex foo matches xxxfoo");
	ok (!_match("foo", "bar"),
        "regex foo does NOT match bar");

	_check_substr_match();

	/* verify that \\n and \\r are converted into \r and \r */
	ok (!_match("foo\\r\\n", "foo\\r\\n"),
        "regex foo\\\\r\\\\n matches foo\\\\r\\\\n");
	ok ( _match("foo\\r\\n", "foo\r\n"),
        "regex foo\\\\r\\\\n does NOT match foo\\r\\n");

	/* check a really long string for a regex */
#define LONG_STR_LEN 	(64*1024*1024)
#define POS_MAGIC 	42
#define POS_WONDERFUL	32*1024*1024
#define POS_COOKIE	63*1024*1024
	s = xmalloc(LONG_STR_LEN);
	memset(s, 'a', LONG_STR_LEN - 1);
	memcpy(s + POS_MAGIC,     "MAGIC",     5);
	memcpy(s + POS_WONDERFUL, "WONDERFUL", 9);
	memcpy(s + POS_COOKIE,    "COOKIE",    6);
	s[LONG_STR_LEN - 1] = '\0';
	ok (_match("MAGIC", s),
        "regex MAGIC matched %d bytes into a string", POS_MAGIC);
	ok (_match("WONDERFUL", s),
        "regex WONDERFUL matched %d bytes into a string", POS_WONDERFUL);
	ok (_match("COOKIE", s),
        "regex COOKIE matched %d bytes into a string", POS_COOKIE);
	ok (!_match("CHOCOLATE", s),
        "regex CHOCOLATE did NOT match in that long string");
	xfree(s);

	/* end of line handling should be disabled since end of string
	 * (which normally matches) is non-deterministic in powerman.
	 * We should be explicitly matching end-of-line sentinels like
	 * \n in scripts.
	 */
    ok (!_match("foo$", "foo"),
        "regex foo$ did NOT match foo");
	ok (!_match("foo$", "foo\n"),
        "regex foo$ did NOT match foo\\n");
	ok (!_match("foo\n", "foo"),
        "regex foo\\n did NOT match foo");
	ok (!_match("foo\n", "bar\nfoo"),
        "regex foo\\n did NOT match bar\\nfoo");
	ok (_match("foo\n", "barfoo\n"),
        "regex foo\\n matched barfoo\\n");

	/* regex takes first match if there are > 1,
	 * but leading wildcard matches greedily
	 */
	ok (_matchstr("foo", "abfoocdfoo", "abfoo"),
        "regex takes first match if there are more than one");
	ok (_matchstr_all(".*foo", "abfoocdfoo"),
        "leading wildcard matches greedily");

	/* check that [:space:] character class works inside bracket
	 * expression
	 */
	ok (_matchstr_all("bar[0-9[:space:]]*foo", "bar  42  foo"),
        "[:space:] character class works inside bracket expression");

	/* debug apcpdu3 regex
	 */
#define B3RX "([0-9])*[^\r\n]*(ON|OFF)\r\n"
	ok (_matchstr_all(B3RX, "     2- Outlet 2                 ON\r\n"),
        "apcpdu3 regex test 1 works");
	ok (_matchstr_all(B3RX, "     9-                          ON\r\n"),
        "apcpdu3 regex test 2 works");

	done_testing();

	exit(0);
}

// vi:ts=4 sw=4 expandtab
