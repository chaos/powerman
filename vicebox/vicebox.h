
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define MAX_BUF 2000              /* an 80x25 screen full of bytes. */
                                  /* wti and baytech have some sreen */
                                  /* oriented display modes */
#define VICEBOX_PORT     10101
#define NUM_ICEBOX_PLUGS   10
#define NUM_NODES          10
#define NUM_ICEBOX_EXPECTS 10
#define MAX_PLUGS          10
#define MAX_NODES          10
#define MAX_EXPECTS        10 /* connect, check connection, disconnect,    */
			      /*   plug query, node query,                 */
			      /*   power on, power off, power cycle, reset,*/
			      /*   error                                   */

#define READ      0 /* fd_set types */
#define WRITE     1
#define EXCEPTION 2

#define TRUE  1 /* bool types */
#define FALSE 0

#define EXP_NOT_FOUND    (-1)
#define EXP_FOUND        1
#define ILLEGAL_EXP      (-2)
#define LOG_IN           0    /* expect indexes */ 
#define CHECK_LOGIN      1
#define LOG_OUT          2
#define PLUG_QUERY       3
#define NODE_QUERY       4
#define POWER_ON         5
#define POWER_OFF        6
#define POWER_CYCLE      7
#define RESET            8
#define EXPECT_ERROR     9


#define TTY_NAME_LEN     11 /* "/dev/ttyS0" is 11 characters with '\0' */
#define ERROR_LIMIT      60
#define TIMEOUT_SECONDS  60

#define LISTEN_BACKLOG    5
#define EMPTY_FD         (-1)

#define LOG                    /* Not implemented yet */
#ifndef NDEBUG
/* Use debugging macros */
void _Assert(char *, unsigned);
#define ASSERT(f)    \
        if(f)        \
        {}           \
        else         \
        exit_error("\nAssertion failed: %s, line %u\n", __FILE__, __LINE__)
#define MAGIC            int magic
#define INIT_MAGIC(x, y) (x)->magic = (y)
#define CLEAR_MAGIC(x)   (x)->magic = (0)
#define BUF_MAGIC     0xdeadbee0
#define EXPECT_MAGIC  0xdeadbee1
#define VICEBOX_MAGIC 0xdeadbee2
#define FD_LIST_MAGIC 0xdeadbee3
#define VALIDATE(x, y) {                                              \
	switch(y) {                                                   \
	case FD_LIST_MAGIC :                                          \
		Valid_FD_List((FD_List *)(x));                        \
		break;                                                \
	case VICEBOX_MAGIC :                                          \
		Valid_Vicebox((Vicebox *)(x));                        \
		break;                                                \
	default :                                                     \
		exit_error("Invalid Validation target: %d\n", (y));   \
	}                                                             \
}
#else
/* Don't use debugging macros */
#define ASSERT(f)
#define MAGIC            
#define INIT_MAGIC(x, y) 
#define CLEAR_MAGIC(x)   
#define VALIDATE(x, y)
#endif

#define MAX(x, y) (((x) > (y))? (x) : (y))
#define MIN(x, y) (((x) < (y))? (x) : (y))

typedef int bool;
typedef struct sockaddr_in Saddr; 


/* 
 * Older iceboxes and WTI units communicated via a serial port (TTY).
 * Newer iceboxes use a variety of RAW TCP communication.  Baytechs and
 * newer WTI units use a telnet listener.  There is some speculation
 * that we may see SNMP based units eventually.  The initial vicebox
 * design will only handle RAW TCP.  I may add TTY if it ends up working
 * in time.  
 */
typedef enum channel_enum {CH_UNKNOWN, TTY, RAW_TCP, TELNET, SNMP} Ch_Type;

typedef enum FD_List_status_struct {NOT_CONNECTED, CONNECTED, 
				    LOGGED_IN}FD_Status;
/* 
 * Each file descriptor is associated with two of these Buffer objects.  
 * They are for the kind of processing presented in Stevens, "UNIX 
 * Network Programming," (ch 15) where he discusses non-blocking reads 
 * and writes.
 */ 
typedef struct buffer_struct {
	MAGIC;
	char buf[MAX_BUF + 1];
	char *start;
	char *end;
	char *in;
	char *out;
} Buffer;
	
/*
 * This is how we represent node and plug state.  The powerman "connected"
 * state is captured here as a boolean in the vicebox structure below.
 */
typedef enum state_enum {ST_UNKNOWN, OFF, ON} State;	

/* 
 * Old fashioned instrumented string (I learned programming in Pascal :^)
 * An expect/send pair is a pair of these.  More general interactions 
 * might have more than one pair, but that is not implemented. 
 */
typedef struct string_struct {
	MAGIC;
	int length;
	char string[MAX_BUF + 1];
} String;


/*
 * The vicebox waits for things that match an expect String and then 
 * reply with a send String.  The Strings themselves have some semantic
 * content so the match and write need to be more abstract than strncmp
 * and strncpy.   
 */
typedef struct expect_struct {
	MAGIC;
	bool in_use;
	String send;
	String expect;
} Expect;

/*
 *  Each open socket holds one of these.  They are maintained in a
 * doubly linked circular list.  There is one distingushed FD_List that
 * is created in the beginning and never deleted.  It is held in the
 * vicebox structure below and named "fds".  It has the illegal file 
 * descriptor value fd == EMPTY_FD == -1.  There is a second 
 * distinguished FD_List also kept in the vicebox structure and named 
 * "listener".  It is no in a list with any other FD_List.  "listener"
 * is for the socket in listen mode.  
 */
typedef struct fd_list_struct {
	MAGIC;
	struct fd_list_struct *next;
	struct fd_list_struct *prev;
/* use these to dynamically set the fd_set objects for select */
	bool read;
	bool write;
	bool exception;
	int fd;
	FD_Status status;
/* the two buffers for non-blocking I/O as per Stevens */
	Buffer to;
	Buffer from;
	Saddr saddr;       /* scratch space for working on the fd */
	int saddr_size;
	int sock_opt;
	int fd_settings;
	struct timeval time;
	int error_count;     /* has this fd been hearing too much noise? */
} FD_List;

/*
 * This structure hold all the state for the virtual icebox as well as 
 * the run time information for managing the non-blocking I/O.
 */
typedef struct vicebox_struct {
	MAGIC;
	char name[MAX_BUF +1];
	char log_file[MAX_BUF +1];
	Ch_Type type;          /* only RAW_TCP for now */
	int tcp_port;          /* hard coded 1010 for now */
	fd_set rset;           /* scratch space for controlling call to */
	fd_set wset;           /*   socket                              */
	fd_set eset;
	FD_List *log;
	FD_List *listener;     /* socket in listen mode */
	FD_List *fds;          /* all the open connections */
	unsigned char off_val;         /* this is all semantic state   */
	unsigned char on_val;          /*   information for the icebox */
	int num_plugs;                 /*   being emulated             */
	State plug_states[MAX_PLUGS];  
	int num_nodes;                 
	State node_states[MAX_PLUGS];
	int num_expects;
	Expect expects[MAX_EXPECTS];   /* description of the send/expect   */
	int longest_expect;            /*   strings for the icbox protocol */
} Vicebox;


/* main module functions (in vicebox.c) */

Vicebox *init_vb(int argc, char **argv);
FD_List *make_listener();
void handle_listener(Vicebox *vice);
void handle_log(Vicebox *vice);
void handle_read(Vicebox *vice, FD_List *fdl);
void handle_write(Vicebox *vice, FD_List *fdl);
/* FD_List handling code */
int max_FD_List(FD_List *head);
void set_FD_List(FD_List *head, fd_set *set, int mode);
FD_List *make_FD_List(int fd);
#ifndef NDEBUG
void Valid_FD_List(FD_List *fdl);
void Valid_Vicebox(Vicebox *v);
#endif
void set_read_FD_List(FD_List *fdl);
void set_noread_FD_List(FD_List *fdl);
void set_write_FD_List(FD_List *fdl);
void set_nowrite_FD_List(FD_List *fdl);
void add_FD_List(FD_List *head, FD_List *fdl);
FD_List *find_FD_List(FD_List *head, int fd);
void del_FD_List(FD_List *head, FD_List *fdl);
/* Other support functions */
void set_string(String *s, const char *cs);
void init_buffer(Buffer *b);
void init_expect(Expect *e);
int find_expect(Vicebox *v, FD_List *fdl, char *str, int len);
void exp_send(Vicebox *v, FD_List *fdl, int msg);
void log_it(Vicebox *v, char *str);
int process_com(Vicebox *v, char *dest, char *src, int max);
int recognize_com(Vicebox *v, char *exp, char *str, int len);
int semantic_action(Vicebox *v, FD_List *fdl, int exp, int node);
/* Wrapper functions (in wrappers.c) */
int Socket(int family, int type, int protocol);
int Setsockopt( int fd, int level, int optname, const void *opt_val, 
		socklen_t optlen );
int Bind( int fd, Saddr *saddr, socklen_t len );
int Getsockopt( int fd, int level, int optname, void *opt_val, 
		socklen_t *optlen );
int Listen(int fd, int backlog);
int Fcntl( int fd, int cmd, int arg);
int Select(int maxfd, fd_set *rset, fd_set *wset, fd_set *eset, 
	    struct timeval *tv);
void clear_sets(fd_set *rset, fd_set *wset, fd_set *eset);
char *Malloc(int size, const char *str);
int Accept(int fd, Saddr *addr, socklen_t *addrlen);
int Read(int fd, char *p, int max);
/* vice_error.c prototype */
void exit_error(const char *fmt, ...);
