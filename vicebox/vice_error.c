#include "vicebox.h"

/*
 * Somewhat like the sys_err() of Stevens, "UNIX Network Programming."
 */

void
exit_error(const char *fmt, ...)
{
	va_list ap;
	char buf[MAX_BUF];
	int er;
	int len;

	er = errno;
	snprintf(buf, MAX_BUF, "Vicebox: ");
	len = strlen(buf);
	va_start(ap, fmt);
	vsnprintf(buf + len, MAX_BUF - len, fmt, ap);
	len = strlen(buf);
	snprintf(buf + len, MAX_BUF - len, ": %s\n", strerror(er));
	fflush(stdout);
	fputs(buf, stderr);
	fflush(stderr);
	va_end(ap);
	exit(1);
}
