#include "buffer.h"
#include "string.h"

#define MAX_MATCH        20

char *util_bufgetregex(Buffer b, regex_t * re);
char *util_bufgetline(Buffer b);
unsigned char *util_findregex(regex_t * re, unsigned char *str, int len);
unsigned char *util_memstr(unsigned char *mem, int len);

/* calculate if time_stamp + timeout > now */
bool util_overdue(struct timeval *time_stamp, struct timeval *timeout);
