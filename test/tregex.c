#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "xtypes.h"
#include "xregex.h"
#include "xmalloc.h"


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
	assert(xregex_exec(re, "xxxfoo1bar2", rm) == TRUE);
	s = xregex_match_sub_strdup(rm, 0);
	assert(s != NULL);
	assert(strcmp(s, "foo1bar2") == 0);
	xfree(s);
	s = xregex_match_sub_strdup(rm, 1);
	assert(s != NULL);
	assert(strcmp(s, "1") == 0);
	xfree(s);
	s = xregex_match_sub_strdup(rm, 2);
	assert(s != NULL);
	assert(strcmp(s, "2") == 0);
	xfree(s);
	s = xregex_match_sub_strdup(rm, 3);
	assert(s == NULL);
	s = xregex_match_sub_strdup(rm, -1); /* powerman actually does this! */
	assert(s == NULL);
	s = xregex_match_strdup(rm);
	assert(strcmp(s, "xxxfoo1bar2") == 0);
	assert(xregex_match_strlen(rm) == strlen(s));
	xfree(s);

	xregex_match_recycle(rm);

	assert(xregex_exec(re, "foobar2", rm) == FALSE);
	s = xregex_match_sub_strdup(rm, 0);
	assert(s == NULL);
	s = xregex_match_sub_strdup(rm, 1);
	assert(s == NULL);

	xregex_match_recycle(rm);

	assert(xregex_exec(re, "xxxfoo1bar2yyy", rm) == TRUE);
	s = xregex_match_sub_strdup(rm, 0);
	assert(s != NULL);
	assert(strcmp(s, "foo1bar2") == 0);
	xfree(s);
	s = xregex_match_sub_strdup(rm, 1);
	assert(s != NULL);
	assert(strcmp(s, "1") == 0);
	xfree(s);
	s = xregex_match_sub_strdup(rm, 2);
	assert(s != NULL);
	assert(strcmp(s, "2") == 0);
	xfree(s);
	s = xregex_match_strdup(rm);
	assert(strcmp(s, "xxxfoo1bar2") == 0);
	assert(xregex_match_strlen(rm) == strlen(s));
	xfree(s);

	xregex_match_destroy(rm);
	xregex_destroy(re);
}

int
main(int argc, char *argv[])
{
	char *s;

	err_init("tregex");

	assert(_match("foo", "foo"));
	assert(_match("foo", "fooxxx"));
	assert(_match("foo", "xxxfoo"));
	assert(!_match("foo", "bar"));

	_check_substr_match();

	/* verify that \\n and \\r are converted into \r and \r */
	assert(!_match("foo\\r\\n", "foo\\r\\n"));
	assert( _match("foo\\r\\n", "foo\r\n"));

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
	assert( _match("MAGIC", s));
	assert( _match("WONDERFUL", s));
	assert( _match("COOKIE", s));
	assert(!_match("CHOCOLATE", s));
	xfree(s);

	/* end of line handling should be disabled since end of string 
	 * (which normally matches) is non-deterministic in powerman.  
	 * We should be explicitly matching end-of-line sentinels like 
	 * \n in scripts.
	 */
	assert(!_match("foo$", "foo"));
	assert(!_match("foo$", "foo\n"));
	assert(!_match("foo\n", "foo"));
	assert(!_match("foo\n", "bar\nfoo"));
	assert( _match("foo\n", "barfoo\n"));

	/* regex takes first match if there are > 1,
	 * but leading wildcard matches greedily
	 */
	assert(_matchstr("foo", "abfoocdfoo", 
                                "abfoo"));
	assert(_matchstr_all(".*foo", "abfoocdfoo"));

	/* check that [:space:] character class works inside bracket
	 * expression
	 */
	assert(_matchstr_all("bar[0-9[:space:]]*foo", "bar  42  foo"));

	/* debug apcpdu3 regex
	 */
#define B3RX "([0-9])*[^\r\n]*(ON|OFF)\r\n"
	assert(_matchstr_all(B3RX, "     2- Outlet 2                 ON\r\n"));
	assert(_matchstr_all(B3RX, "     9-                          ON\r\n"));

	exit(0);
}
