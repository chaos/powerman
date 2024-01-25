#ifndef PM_XREAD_H
#define PM_XREAD_H

/* Read function that handles EINTR internally and terminates on all
 * other errors except EWOULDBLOCK and ECONNRESET.
 */
int xread(int fd, char *p, int max);

/* Write function that handles EINTR internally and terminates on all
 * other errors except EPIPE, EWOULDBLOCK, and ECONNRESET.
 */
int xwrite(int fd, char *p, int max);

/* Wrapper for xread that reads exactly count bytes or dies trying
 */
void xread_all(int fd, char *p, int count);

/* Wrapper for xwrite that writes exactly count bytes or dies trying.
 */
void xwrite_all(int fd, char *p, int count);

/* Read a line of input, strip whitespace off the end, and return it.
 */
char *xreadline(char *prompt, char *buf, int buflen);

/* Read a line of data from file descriptor, terminated with \r\n,
 * which is not returned.  Exit on EOF or read error.
 * Caller must free returned string (null terminated).
 */
char *xreadstr(int fd);

#endif /* PM_XREAD_H */


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
