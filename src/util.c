#include <stdio.h>

#include "powerman.h"

#include "buffer.h"
#include "pm_string.h"

/*
 * Wrapper for get_str_from_Buffer that returns result as a String type,
 * or NULL if nothing found.
 * NOTE: this is here to avoid making either pm_string and buffer modules
 * dependent on the other.
 */
String
get_String_from_Buffer(Buffer b, regex_t *re)
{
	char str[MAX_BUF];
	int res = get_str_from_Buffer(b, re, str, MAX_BUF);

	return (res > 0 ? make_String(str) : NULL);
}
