bool pipe_connect(Device * dev);
void pipe_disconnect(Device * dev);
void *pipe_create(char *cmdline, char *flags);
void pipe_destroy(void *data);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
