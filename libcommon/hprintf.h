#ifndef PM_HPRINTF_H
#define PM_HPRINTF_H

/* A vsprintf-like function that allocates the string on the heap and ensures
 * that no truncation occurs.  The caller must Free() the resulting string.
 */
char *hvsprintf(const char *fmt, va_list ap);

/* An sprintf-like function that allocates the string on the heap.
 * The caller must Free() the resulting string.
 */
char *hsprintf(const char *fmt, ...);

int hfdprintf(int fd, const char *format, ...);

#endif /* PM_HPRINTF_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
