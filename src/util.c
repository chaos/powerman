#include <stdio.h>
#include <sys/types.h>
#include <regex.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "powerman.h"
#include "wrappers.h"
#include "util.h"

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
 * vi:softtabstop=4
 */
