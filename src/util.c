#include <stdio.h>
#include <sys/types.h>
#include <regex.h>

#include "powerman.h"

#include "buffer.h"
#include "pm_string.h"
#include "util.h"
#include "wrappers.h"

/* 
 * Look for 're' in the first 'len' bytes of 'str' and return it's 
 * position if found or NULL if not found.
 */
char *
find_RegEx(regex_t *re, char *str, int len)
{
	int n;
	size_t nmatch = MAX_MATCH;
	regmatch_t pmatch[MAX_MATCH];
	int eflags = 0;

	n = Regexec(re, str, nmatch, pmatch, eflags);
	if (n != REG_NOERROR) return NULL;
	if ((pmatch[0].rm_so < 0) || (pmatch[0].rm_eo > len))
		return NULL;
	else
		return str + pmatch[0].rm_eo;
}

String
get_line_from_Buffer(Buffer b)
{
	char str[MAX_BUF];
	int res = get_line_Buffer(b, str, MAX_BUF);

	return (res > 0 ? make_String(str) : NULL);
}

String
get_String_from_Buffer(Buffer b, regex_t *re)
{
	char str[MAX_BUF];
	int bytes_peeked = peek_string_Buffer(b, str, MAX_BUF);
	bool match = FALSE;

	if (bytes_peeked > 0 && find_RegEx(re, str, bytes_peeked) != NULL)
	{
		eat_Buffer(b, bytes_peeked);
		match = TRUE;
	}
	return (match ? make_String(str) : NULL);
}
