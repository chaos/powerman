#ifndef PM_XSIGNAL_H
#define PM_XSIGNAL_H

typedef void xsigfunc_t(int);
xsigfunc_t *xsignal(int signo, xsigfunc_t * func);

#endif /* PM_XSIGNAL_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
