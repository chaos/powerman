bool serial_connect(Device * dev);
void serial_disconnect(Device * dev);
void *serial_create(char *special, char *flags);
void serial_destroy(void *data);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
