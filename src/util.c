#include <stdio.h>
#include <sys/types.h>
#include <regex.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "powerman.h"

#include "buffer.h"
#include "pm_string.h"
#include "util.h"
#include "wrappers.h"

/* 
 * Look for 're' in the first 'len' bytes of 'str' and return a pointer
 * to the character following the last character of the match,
 * or NULL if no match.
 */
unsigned char *find_RegEx(regex_t * re, unsigned char *str, int len)
{
    int n;
    size_t nmatch = MAX_MATCH;
    regmatch_t pmatch[MAX_MATCH];
    int eflags = 0;

    n = Regexec(re, str, nmatch, pmatch, eflags);
    if (n != REG_NOERROR)
	return NULL;
    if (pmatch[0].rm_so == -1 || pmatch[0].rm_eo == -1)
	return NULL;
    assert(pmatch[0].rm_eo <= len);
    return (str + pmatch[0].rm_eo);
}

/*
 * A wrapper for get_line_Buffer that returns a String type
 * that athe caller must free.
 */
String get_line_from_Buffer(Buffer b)
{
    unsigned char str[MAX_BUF];
    int res = get_line_Buffer(b, str, MAX_BUF);

    return (res > 0 ? make_String(str) : NULL);
}

/*
 * Apply regular expression to the contents of a Buffer.
 * If there is a match, return (and consume) from the beginning
 * of the buffer to the last character of the match.
 * NOTE: embedded \0 chars are converted to \377 by get_*_Buffer() and
 * peek_*_Buffer9() because libc regex functions would treat these as string 
 * terminators.  As a result, \0 chars cannot be matched explicitly.
 */
String get_String_from_Buffer(Buffer b, regex_t * re)
{
    unsigned char str[MAX_BUF];
    int bytes_peeked = peek_string_Buffer(b, str, MAX_BUF);
    unsigned char *match_end;

    if (bytes_peeked == 0)
	return NULL;
    match_end = find_RegEx(re, str, bytes_peeked);
    if (match_end == NULL)
	return NULL;
    assert(match_end - str <= strlen(str));
    *match_end = '\0';
    eat_Buffer(b, match_end - str);	/* only consume up to what matched */

    return make_String(str);
}

/*
 * Convert memory to string, turning non-printable character into "C" 
 * representation.  Call Free() on the result to free its storage.
 */
unsigned char *memstr(unsigned char *mem, int len)
{
    int i, j;
    int strsize = len * 4;	/* worst case */
    unsigned char *str = Malloc(strsize + 1);

    for (i = j = 0; i < len; i++) {
	switch (mem[i]) {
	case '\r':
	    strcpy(&str[j], "\\r");
	    j += 2;
	    break;
	case '\n':
	    strcpy(&str[j], "\\n");
	    j += 2;
	    break;
	case '\t':
	    strcpy(&str[j], "\\t");
	    j += 2;
	    break;
	default:
	    if (isprint(mem[i]))
		str[j++] = mem[i];
	    else {
		sprintf(&str[j], "\\%.3o", mem[i]);
		j += 4;
	    }
	    break;
	}
	assert(j <= strsize || i == len);
    }
    str[j] = '\0';
    return str;
}
