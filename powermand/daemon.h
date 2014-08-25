#ifndef PM_DAEMON_H
#define PM_DAEMON_H

void daemon_init(int *skipfds, int skipfdslen, char *rundir, char *pidfile,
                 char *logname);


#endif /* PM_DAEMON_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
