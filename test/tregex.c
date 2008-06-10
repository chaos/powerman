#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "xtypes.h"
#include "xregex.h"
#include "xmalloc.h"

#define LONG_STR_LEN 	(64*1024*1024)
#define POS_MAGIC 	42
#define POS_WONDERFUL	32*1024*1024
#define POS_COOKIE	63*1024*1024

int
main(int argc, char *argv[])
{
	xregex_t re; 
	xregex_match_t rm;
	char *s;

	re = xregex_create();
	xregex_compile(re, "foo", FALSE);
	assert(xregex_exec(re, "foo", NULL) == TRUE);
	assert(xregex_exec(re, "fooxxx", NULL) == TRUE);
	assert(xregex_exec(re, "xxxfoo", NULL) == TRUE);
	assert(xregex_exec(re, "bar", NULL) == FALSE);
	xregex_destroy(re);

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

	/* verify that \\n and \\r are converted into \r and \r */
	re = xregex_create();
	xregex_compile(re, "foo\\r\\n", FALSE);
	assert(xregex_exec(re, "foo\\r\\n", NULL) == FALSE);
	assert(xregex_exec(re, "foo\r\n", NULL) == TRUE);
	xregex_destroy(re);

	/* check a really long string for a regex */
	s = xmalloc(LONG_STR_LEN);
	memset(s, 'a', LONG_STR_LEN - 1);
	memcpy(s + POS_MAGIC,     "MAGIC",     5);
	memcpy(s + POS_WONDERFUL, "WONDERFUL", 9);
	memcpy(s + POS_COOKIE,    "COOKIE",    6);
	s[LONG_STR_LEN - 1] = '\0';

	re = xregex_create();
	xregex_compile(re, "MAGIC", FALSE);
	assert(xregex_exec(re, s, NULL) == TRUE);
	xregex_destroy(re);

	re = xregex_create();
	xregex_compile(re, "WONDERFUL", FALSE);
	assert(xregex_exec(re, s, NULL) == TRUE);
	xregex_destroy(re);

	re = xregex_create();
	xregex_compile(re, "COOKIE", FALSE);
	assert(xregex_exec(re, s, NULL) == TRUE);
	xregex_destroy(re);

	re = xregex_create();
	xregex_compile(re, "CHOCOLATE", FALSE);
	assert(xregex_exec(re, s, NULL) == FALSE);
	xregex_destroy(re);

	xfree(s);

	exit(0);
}
