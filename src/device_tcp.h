bool tcp_finish_connect(Device * dev);
bool tcp_connect(Device * dev);
void tcp_disconnect(Device * dev);
void tcp_preprocess(Device * dev);
void *tcp_create(char *host, char *port);
void tcp_destroy(void *data);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

