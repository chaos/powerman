#ifndef PM_DEVICE_H
#define PM_DEVICE_H

void dev_init(bool short_circuit_delay);
void dev_fini(void);
void dev_initial_connect(void);

void dev_pre_poll(xpollfd_t pfd);
void dev_post_poll(xpollfd_t pfd, struct timeval *tv);

#endif /* PM_DEVICE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
