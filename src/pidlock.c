#include <fcntl.h>
#include <string.h>

#include "exit_error.h"
#if 0
void
pid_write(void)
{
	int n;
	int fd;

	n = mkdir(PID_DIR, S_IRWXU);
	if( (n == -1) && (errno != EEXIST) )
		exit_error("mkdir %s", PID_DIR);
	fd = open(PID_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if( fd == -1 ) 
		exit_error("open %s", PID_FILE_NAME);
	sprintf(pidstr, "%d\n", pid = getpid());
	len = strlen(pidstr);
	n = write(fd, pidstr, len);
	if( n != len ) 
		exit_msg("Failed to write out pid value");
	Close(fd);
}


/*
 *   Both SIGTERM and SIGHUP may be sent to the process id listed in
 * the location PID_FILE_NAME (/var/run/powerman/powerman.pid), but only
 * SIGTERM actually works.  If the pid file is missing the program
 * exits with an error, and no process is signalled.  If the pid file
 * is found it is deleted whether the process is found or not.
 *
 * FIXME: I'd prefer to have /var/run/powerman/powerman.<pid> and 
 * allow for multiple daemons to coexist and be manageable.  "-k"
 * would presumably sequence through sending "TERM" to all of them.
 * There can be multiple powemand instances but currently "-k" will
 * only TERM the last one started.
 */
static void
pid_signal(int signum)
{
	FILE *fp;
	pid_t pid;
	int n;

	fp = fopen(PID_FILE_NAME, "r");
	if( fp == NULL ) 
		exit_error("%s", PID_FILE_NAME);
	n = fscanf(fp, "%d", &pid);
	if( n != 1 ) 
		exit_msg("failed to find a pid in %s", PID_FILE_NAME); 
	n = kill(pid, signum);
	if( n < 0 ) 
		exit_error("kill -%d %d", signum, pid);
	unlink(PID_FILE_NAME);
}
#endif
