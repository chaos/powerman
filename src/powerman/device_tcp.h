#ifndef PM_DEVICE_TCP_H
#define PM_DEVICE_TCP_H

bool tcp_finish_connect(Device * dev);
bool tcp_connect(Device * dev);
void tcp_disconnect(Device * dev);
void tcp_preprocess(Device * dev);
void *tcp_create(char *host, char *port, char *flags);
void tcp_destroy(void *data);

#endif /* PM_DEVICE_TCP_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

