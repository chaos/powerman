#ifndef PM_XMALLOC_H
#define PM_XMALLOC_H

char *xmalloc(int size);
char *xrealloc(char *item, int newsize);
void xfree(void *ptr);
char *xstrdup(const char *str);
int xmemory(void);

#endif /* PM_XMALLOC_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
