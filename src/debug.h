#ifndef DEBUG_H
#define DEBUG_H

#define DBG_ALWAYS          0x0000
#define DBG_DEVICE          0x0001
#define DBG_SELECT          0x0002
#define DBG_CLIENT          0x0004
#define DBG_ACTION          0x0008
#define DBG_MEMORY          0x0010
#define DBG_TELNET          0x0020

#define DBG_NAME_TAB {                      \
    { DBG_DEVICE,       "device" },         \
    { DBG_SELECT,       "select" },         \
    { DBG_CLIENT,       "client" },         \
    { DBG_ACTION,       "action" },         \
    { DBG_MEMORY,       "memory" },         \
    { DBG_TELNET,       "telnet" },         \
    { 0, NULL }                             \
    }

void dbg_notty(void);
void dbg_setmask(unsigned long mask);
void _dbg(unsigned long channel, const char *fmt, ...);
char *dbg_fdsetstr(fd_set * set, int n, char *str, int len);
char *dbg_tvstr(struct timeval *tv, char *str, int len);
unsigned char *dbg_memstr(unsigned char *mem, int len);

#ifdef NDEBUG
#define dbg(channel, fmt...)
#else
#define dbg(channel, fmt...)    _dbg(channel, fmt)
#endif

#endif /* DEBUG_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
