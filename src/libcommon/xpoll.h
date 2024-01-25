#ifndef PM_XPOLL_H
#define PM_XPOLL_H

typedef struct xpollfd *xpollfd_t;

int         xpoll(xpollfd_t pfd, struct timeval *timeout);
xpollfd_t   xpollfd_create(void);
void        xpollfd_destroy(xpollfd_t pfd);
void        xpollfd_zero(xpollfd_t pfd);
void        xpollfd_set(xpollfd_t pfd, int fd, short events);
short       xpollfd_revents(xpollfd_t pfd, int fd);
char       *xpollfd_str(xpollfd_t pfd, char *str, int len);

#define XPOLLIN      1
#define XPOLLOUT     2
#define XPOLLHUP     4
#define XPOLLERR     8
#define XPOLLNVAL    16

#endif /* PM_XPOLL_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
