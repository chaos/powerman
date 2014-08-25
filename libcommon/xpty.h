#ifndef PM_XPTY_H
#define PM_XPTY_H

/* FIXME: these have nothing to do with ptys per se */
void xcfmakeraw(int fd);
void nonblock_set(int fd);
void nonblock_clr(int fd);

pid_t xforkpty(int *amaster, char *name, int len);

#endif /* PM_XPTY_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
