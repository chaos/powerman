#ifndef DEBUG_H
#define DEBUG_H

#define DBG_ALWAYS		0x0000
#define DBG_DEVICE		0x0001
#define DBG_SELECT		0x0002
#define DBG_CLIENT		0x0004
#define DBG_ACTION		0x0008
#define DBG_BUFFER		0x0010
#define DBG_DEV_TELEMETRY 	0x0020
#define DBG_CLI_TELEMETRY 	0x0040
#define DBG_SCRIPT 		0x0080
#define DBG_MEMORY		0x0100

#define DBG_NAME_TAB { 				\
	{ DBG_DEVICE,		"device" },	\
	{ DBG_SELECT, 		"select" },	\
	{ DBG_CLIENT, 		"client" },	\
	{ DBG_BUFFER, 		"buffer" },	\
	{ DBG_SCRIPT, 		"script" },	\
	{ DBG_DEV_TELEMETRY,	"dev_telemetry" }, \
	{ DBG_CLI_TELEMETRY,	"cli_telemetry" }, \
	{ DBG_SCRIPT, 		"script" },	\
	{ DBG_MEMORY, 		"memory" },	\
	{ DBG_ACTION, 		"action" },	\
	{ 0, 			NULL }		\
}

void dbg_notty(void);
void dbg_setmask(unsigned long mask);
void dbg(unsigned long channel, const char *fmt, ...);
char *dbg_fdsetstr(fd_set *set, int n, char *str, int len);
char *dbg_tvstr(struct timeval *tv, char *str, int len);
unsigned char *dbg_memstr(unsigned char *mem, int len);

#endif /* DEBUG_H */
