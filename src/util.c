#include <stdio.h>
#include <sys/types.h>
#include <regex.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "powerman.h"

#include "buffer.h"
#include "string.h"
#include "util.h"
#include "wrappers.h"

/* 
 * Find regular expression in string.
 *  re (IN)	regular expression
 *  str (IN)	where to look for regular expression
 *  len (IN)	number of chars in str to search
 *  RETURN	pointer to char following the last char of the match,
 *		or NULL if no match
 */
unsigned char *util_findregex(regex_t * re, unsigned char *str, int len)
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
 * A wrapper for buf_getline that returns a String type.
 *  b (IN)	target Buffer
 *  RETURN	line from buffer (caller must free)
 */
String util_bufgetline(Buffer b)
{
    unsigned char str[MAX_BUF];
    int res = buf_getline(b, str, MAX_BUF);

    return (res > 0 ? str_create(str) : NULL);
}

/*
 * Apply regular expression to the contents of a Buffer.
 * If there is a match, return (and consume) from the beginning
 * of the buffer to the last character of the match.
 * NOTE: embedded \0 chars are converted to \377 by buf_getstr/buf_getline and
 * buf_peekstr/buf_getstr because libc regex functions would treat these as 
 * string terminators.  As a result, \0 chars cannot be matched explicitly.
 *  b (IN)	buffer to apply regex to
 *  re (IN)	regular expression
 *  RETURN	String match (caller must free) or NULL if no match
 */
String util_bufgetregex(Buffer b, regex_t * re)
{
    unsigned char str[MAX_BUF];
    int bytes_peeked = buf_peekstr(b, str, MAX_BUF);
    unsigned char *match_end;

    if (bytes_peeked == 0)
	return NULL;
    match_end = util_findregex(re, str, bytes_peeked);
    if (match_end == NULL)
	return NULL;
    assert(match_end - str <= strlen(str));
    *match_end = '\0';
    buf_eat(b, match_end - str);	/* only consume up to what matched */

    return str_create(str);
}

/*
 * Convert memory to string, turning non-printable character into "C" 
 * representation.  
 *  mem (IN)	target memory
 *  len (IN)	number of characters to convert
 *  RETURN	string (caller must free)
 */
unsigned char *util_memstr(unsigned char *mem, int len)
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

/*
 * Test whether timeout has occurred
 *  time_stamp (IN) 
 *  timeout (IN)
 *  RETURN		TRUE if (time_stamp + timeout > now)
 */
bool util_overdue(struct timeval * time_stamp, struct timeval * timeout)
{
    struct timeval now;
    struct timeval limit;
    bool result = FALSE;

    /* timeradd(time_stamp, timeout, limit) */
    limit.tv_usec = time_stamp->tv_usec + timeout->tv_usec;
    limit.tv_sec = time_stamp->tv_sec + timeout->tv_sec;
    if (limit.tv_usec > 1000000) {
	limit.tv_sec++;
	limit.tv_usec -= 1000000;
    }
    gettimeofday(&now, NULL);
    /* timercmp(now, limit, >) */
    if ((now.tv_sec > limit.tv_sec) ||
	((now.tv_sec == limit.tv_sec) && (now.tv_usec > limit.tv_usec)))
	result = TRUE;
    return result;
}


/*
 * vi:softtabstop=4
 */
