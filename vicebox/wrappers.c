#include "vicebox.h"

/*
 *  Taken nearly verbatim from Stevens, "UNIX Network Programming".
 *  Some are my own extrapolations of his ideas, but should be
 * immediately obvious.  
 */

int
Socket(int family, int type, int protocol)
{
	int fd;

	fd = socket(family, type, protocol);
	if(fd < 0) 
		exit_error("Could not open socket");
	return fd;
}

int
Setsockopt( int fd, int level, int optname, const void *opt_val, 
	    socklen_t optlen ) 
{
	int ret_code;

	ret_code = setsockopt(fd, level, optname, 
			      opt_val, optlen);
	if(ret_code < 0) 
		exit_error("Could not set socket option");
	return ret_code;
}

int
Bind( int fd, Saddr *saddr, socklen_t len )
{
	int ret_code;

	ret_code = bind(fd, (struct sockaddr *)saddr, len);
	if(ret_code < 0) 
		exit_error("bind problem");
	return ret_code;
}

int
Getsockopt( int fd, int level, int optname, void *opt_val, 
	    socklen_t *optlen ) 
{
	int ret_code;

	ret_code = getsockopt(fd, level, optname, 
			      opt_val, optlen);
	if(ret_code < 0) 
		exit_error("Could not get socket info");
	return ret_code;
}

int
Listen(int fd, int backlog)
{
	int ret_code;

	ret_code = listen( fd, backlog );
	if(ret_code < 0) 
		exit_error("listen problem");
	return ret_code;
}

int
Fcntl( int fd, int cmd, int arg)
{
	int ret_code;

	ret_code = fcntl(fd, cmd, arg);
	if(ret_code < 0) 
		exit_error("fcntl problem");
	return ret_code;
}

int
Select(int maxfd, fd_set *rset, fd_set *wset, fd_set *eset, struct timeval *tv)
{
	int n;
	static int error_count = 0;

	n = select(maxfd, rset, wset, eset, tv);
	if (n < 0)
	{
/* Some sort of error occured.  Can we ignode it? */
		if (errno != EINTR) 
			exit_error("select error");
		clear_sets(rset, wset, eset);
	}
	if (n == 0)
                /* Timed out.  */
		clear_sets(rset, wset, eset);
	if (n > 0)
/* There is some reading or writing bit set, so reset the error count */
		error_count = 0;
	return n;
}

void
clear_sets(fd_set *rset, fd_set *wset, fd_set *eset)
{
	if(rset != NULL) FD_ZERO(rset);
	if(wset != NULL) FD_ZERO(wset);
	if(eset != NULL) FD_ZERO(eset);
}

char *
Malloc(int size, const char *str)
{
	char *new;
	new = malloc(size);
	if (new == NULL)
		exit_error("Out of memory creating %s", str);
	return new;
}

int
Accept(int fd, Saddr *addr, socklen_t *addrlen)
{
	int new;
	new = accept(fd, (struct sockaddr *)addr, addrlen);
	if(new < 0) 
	{
/* 
 *   A client could abort before a ready connection is accepted.  "The fix
 * for this problem is to:  ...  2.  "Ignore the following errors ..."
 *                                                     - Stevens, UNP p424
 */
		if ( !( (errno == EWOULDBLOCK)  ||
			(errno == ECONNABORTED) ||
			(errno == EPROTO)       ||
			(errno == EINTR) ) )
			exit_error("accept problem");
	}
	return new;
}

int
Read(int fd, char *p, int max)
{
	int n;

	n = read(fd, p, max);
	if (( n < 0 ) && 
	    (( errno != EWOULDBLOCK ) &&
	     ( errno != ECONNRESET )))
		exit_error("Read error");
	return n;
}
