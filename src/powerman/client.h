#ifndef PM_CLIENT_H
#define PM_CLIENT_H

void cli_init(void);
void cli_fini(void);

void cli_start(bool use_stdio, bool one_client);
void cli_listen_fds(int **fds, int *len);
bool cli_server_done(void);

void cli_post_poll(xpollfd_t pfd);
void cli_pre_poll(xpollfd_t pfd);

#endif /* PM_CLIENT_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
