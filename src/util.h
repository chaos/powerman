#include "buffer.h"
#include "string.h"

#define MAX_MATCH        20

String util_bufgetregex(Buffer b, regex_t * re);
String util_bufgetline(Buffer b);
unsigned char *util_findregex(regex_t * re, unsigned char *str, int len);
unsigned char *util_memstr(unsigned char *mem, int len);
