#include "buffer.h"
#include "pm_string.h"

#define MAX_MATCH        20

String get_String_from_Buffer(Buffer b, regex_t *re);
String get_line_from_Buffer(Buffer b);
unsigned char *find_RegEx(regex_t *re, unsigned char *str, int len);
unsigned char *memstr(unsigned char *mem, int len);
