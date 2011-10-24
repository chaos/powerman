/*****************************************************************************
 *  Copyright (C) 2007-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>
 *  UCRL-CODE-2002-008.
 *  
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see <http://www.llnl.gov/linux/powerman/>.
 *  
 *  PowerMan is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  PowerMan is distributed in the hope that it will be useful, but WITHOUT 
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 *  for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with PowerMan; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

/* plmpower.c - control Insteon/X10 devices via SmartLabs PLM 2412S */

/* References:
 * For PLM bits, see "Insteon Modem Developer's Guide",
 *   http://www.smarthome.com/manuals/2412sdevguide.pdf
 * For general Insteon protocol, see "Insteon: The Details"
 *   http://www.insteon.net/pdf/insteondetails.pdf
 * For Insteon commands, see "EZBridge Reference Manual", Appendix B.
 *   http://www.simplehomenet.com/Downloads/EZBridge%20Manual.pdf
 * Also: "Quick Reference Guide for Smarthome Device Manager for INSTEON"
 *   http://www.insteon.net/sdk/files/dm/docs/
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <libgen.h>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <sys/time.h>

#include "xtypes.h"
#include "xmalloc.h"
#include "error.h"
#include "argv.h"
#include "xread.h"
#include "xtime.h"
#include "xpoll.h"

/* PLM commands */
#define IM_RECV_STD                 0x50    /* plm -> host */
#define IM_RECV_EXT                 0x51
#define IM_RECV_X10                 0x52
#define IM_RECV_ALL_LINK_COMPLETE   0x53
#define IM_RECV_BUTTON              0x54
#define IM_RECV_RESET               0x55
#define IM_RECV_ALL_LINK_FAIL       0x56
#define IM_RECV_ALL_LINK_REC        0x57
#define IM_RECV_ALL_LINK_STAT       0x58
#define IM_GET_INFO                 0x60    /* host -> plm */
#define IM_SEND_ALL_LINK            0x61
#define IM_SEND                     0x62
#define IM_SEND_X10                 0x63
#define IM_START_ALL_LINK           0x64
#define IM_CANCEL_ALL_LINK          0x65
#define IM_SET_HOST_CATEGORY        0x66
#define IM_RESET                    0x67
#define IM_SET_ACK                  0x68
#define IM_GET_FIRST_ALL_LINK_REC   0x69
#define IM_GET_NEXT_ALL_LINK_REC    0x6A
#define IM_CONFIG_SET               0x6B
#define IM_GET_SENDER_ALL_LINK_REC  0x6C
#define IM_LED_ON                   0x6D
#define IM_LED_OFF                  0x6E
#define IM_MANAGE_ALL_LINK_REC      0x6F
#define IM_SET_NACK                 0x70
#define IM_SET_ACK2                 0x71
#define IM_RF_SLEEP                 0x72
#define IM_CONFIG_GET               0x73

/* PLM control bytes */
#define IM_STX                      0x02
#define IM_ACK                      0x06
#define IM_NAK                      0x15

/* PLM config bits */
#define IM_CONFIG_BUTLINK_DISABLE   0x80
#define IM_CONFIG_MONITOR_MODE      0x40
#define IM_CONFIG_AUTOLED_DISABLE   0x20
#define IM_CONFIG_DEADMAN_DISABLE   0x10

#define IM_MAX_RECVLEN              28 /* response to send extended msg */

/* Insteon commands (incomplete) */
#define CMD_GRP_ASSIGN              0x01
#define CMD_GRP_DELETE              0x02
#define CMD_PING                    0x10
#define CMD_ON                      0x11
#define CMD_ON_FAST                 0x12
#define CMD_OFF                     0x13
#define CMD_OFF_FAST                0x14
#define CMD_BRIGHT                  0x15
#define CMD_DIM                     0x16
#define CMD_MAN_START               0x17
#define CMD_MAN_STOP                0x18
#define CMD_STATUS                  0x19

/* X10 commands */
#define X10_ALL_UNITS_OFF           0x0
#define X10_ALL_LIGHTS_ON           0x1
#define X10_ON                      0x2
#define X10_OFF                     0x3
#define X10_DIM                     0x4
#define X10_BRIGHT                  0x5
#define X10_ALL_LIGHTS_OFF          0x6
#define X10_EXT_CODE                0x7
#define X10_HAIL_REQ                0x8
#define X10_HAIL_ACK                0x9
#define X10_PRESET_DIM              0xa
#define X10_PRESET_DIM2             0xb
#define X10_EXT_DATA                0xc
#define X10_STATUS_ON               0xd
#define X10_STATUS_OFF              0xe
#define X10_STATUS_REQ              0xf

/* X10 encoding of housecode A-P and device code 1-16 */
static const int x10_enc[] = { 
    0x6, 0xe, 0x2, 0xa, 0x1, 0x9, 0x5, 0xd, 
    0x7, 0xf, 0x3, 0xb, 0x0, 0x8, 0x4, 0xc 
};

typedef struct {
    char h;
    char m;
    char l;
} insaddr_t;

typedef struct {
    char house;
    char unit;
} x10addr_t;

static void     usage(void);
static int      open_serial(char *dev);
static int      run_cmd(int fd, char *cmd);
static void     shell(int fd);
static void     docmd(int fd, char **av, int *quitp);
static void     help(void);
static void     plm_reset(int fd);
static void     plm_info(int fd);
static void     plm_on(int fd, char *addrstr);
static void     plm_off(int fd, char *addrstr);
static void     plm_status(int fd, char *addrstr);
static void     plm_ping(int fd, char *addrstr);

static int      str2insaddr(char *s, insaddr_t *ip);
static char    *insaddr2str(insaddr_t *ip);
static int      str2x10addr(char *s, x10addr_t *xp);
static int      wait_until_ready(int fd, int timeout_msec);
static int      plm_recv(int fd, char cmd, char *recv, int recvlen);
static void     plm_docmd(int fd, char *send, int sendlen, 
                          char *recv, int recvlen);
static void     plm_send_insteon(int fd, insaddr_t *ip, char cmd1, char cmd2);
static int      plm_recv_insteon(int fd, insaddr_t *ip, char *cmd1, char *cmd2,
                                 int timeout_msec);
static void     plm_send_x10(int fd, x10addr_t *xp, char cmd);

static int              testmode = 0;
static char             test_plug = 0;
static unsigned long    x10_attempts = 3;
static int              insteon_tmout = 1000; /* (msec) */

#define OPTIONS "d:Tx:t:S:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    { "device",         required_argument, 0, 'd' },
    { "testmode",       no_argument,       0, 'T' },
    { "x10-attempts",   required_argument, 0, 'x' },
    { "timeout",        required_argument, 0, 't' },
    { "single-cmd",     required_argument, 0, 'S' },
    {0,0,0,0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int
main(int argc, char *argv[])
{
    char *device = NULL;
    int c;
    int fd = -1;
    char *single_cmd = NULL;

    err_init(basename(argv[0]));

    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'd':   /* --device */
                device = optarg;
                break;
            case 'T':   /* --testmode */
                testmode = 1;
                break;
            case 't':   /* --timeout */
                insteon_tmout = strtoul(optarg, NULL, 10);
                if (insteon_tmout < 1)
                    err_exit(FALSE, "timeout must be >= 1 msec");
                break;
            case 'x':   /* --x10-attempts */
                x10_attempts = strtoul(optarg, NULL, 10);
                if (x10_attempts < 1)
                    err_exit(FALSE, "X10 attempts must be >= 1");
                break;
            case 'S':   /* --single-cmd */
                single_cmd = strdup(optarg);
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc)
        usage();

    if (!testmode) {
        if (device == NULL)
            usage();
        fd = open_serial(device);
    }

    if (single_cmd) {
        run_cmd(fd, single_cmd);
        free(single_cmd);
    } else
        shell(fd);


    if (close(fd) < 0)
        err_exit(TRUE, "error closing %s", device);
    exit(0);
}

static void
usage(void)
{
    fprintf(stderr, "Usage: plmpower -d device \n");
    exit(1);
}

/* Open the PLM's serial device [dev], lock it in a manner consistent
 * with powerman and conman, set baud rate, line discipline, etc,
 * and return file descriptor.
 */
static int
open_serial(char *dev)
{
    struct termios tio;
    int fd;

    fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0)
        err_exit(TRUE, "could not open %s", dev);
    if (lockf(fd, F_TLOCK, 0) < 0)
        err_exit(TRUE, "could not lock %s", dev);
    tcgetattr(fd, &tio);
    tio.c_cflag = B19200 | CS8 | CLOCAL | CREAD;
    tio.c_iflag = IGNBRK | IGNPAR;
    tio.c_oflag = ONLRET;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 1;
    tcsetattr(fd, TCSANOW, &tio);

    return fd;
}

/* Parse the command line into an argv array which is passed to docmd() for
 * execution against PLM on [fd].  Returns if the user wants to quit.
 */
static int
run_cmd(int fd, char *cmd)
{
    int quit = 0;
    char **av;
    av = argv_create(cmd, "");
    docmd(fd, av, &quit);
    argv_destroy(av);
    return (quit);
}

/* Prompt the user and run the command entered.
 * Returns when the user wants to quit.
 */
static void shell(int fd)
{
    char buf[128];
    int quit = 0;

    while (!quit) {
        printf("plmpower> ");
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin)) {
            quit = run_cmd(fd, buf);
        } else
            quit = 1;
    }
}

/* Parse argv array [av] and execute any commands against PLM on [fd].
 * If the user wants to quit, set [*quitp] to 1.
 */
static void 
docmd(int fd, char **av, int *quitp)
{
    if (av[0] != NULL) {
        if (strcmp(av[0], "help") == 0) {
            help();
        } else if (strcmp(av[0], "quit") == 0) {
            *quitp = 1;
        } else if (strcmp(av[0], "info") == 0) {
            if (testmode)
                printf("Unavailable in test mode\n");
            else
                plm_info(fd);
        } else if (strcmp(av[0], "reset") == 0) {
            if (testmode)
                printf("Unavailable in test mode\n");
            else
                plm_reset(fd);
        } else if (strcmp(av[0], "on") == 0) {
            if (argv_length(av) != 2)
                printf("Usage: on addr\n");
            else
                plm_on(fd, av[1]);
        } else if (strcmp(av[0], "off") == 0) {
            if (argv_length(av) != 2)
                printf("Usage: off addr\n");
            else
                plm_off(fd, av[1]);
        } else if (strcmp(av[0], "status") == 0) {
            if (argv_length(av) != 2)
                printf("Usage: status addr\n");
            else
                plm_status(fd, av[1]);
        } else if (strcmp(av[0], "ping") == 0) {
            if (testmode)
                printf("Unavailable in test mode\n");
            else if (argv_length(av) != 2)
                printf("Usage: ping addr\n");
            else
                plm_ping(fd, av[1]);
        } else 
            printf("type \"help\" for a list of commands\n");
    }
}

/* Summarize interactive "shell" commands.
 */
static void 
help(void)
{
    printf("Valid commands are:\n");
    printf("  info             get PLM info\n");
    printf("  reset            reset the PLM (clears all-link db)\n");
    printf("  on     addr      turn on device\n");
    printf("  off    addr      turn on device\n");
    printf("  status addr      query status of device\n");
    printf("  ping   addr      time round trip request/response to device\n");
    printf("Where addr is Insteon (e.g. 1A.2B.3C) or X10 (e.g. G12)\n");
}

/* Send the ON command to [addrstr], either X10 or Insteon, via PLM on [fd].
 * If Insteon, retry the command until a response is received.
 * If X10, send it x10_attempts times whether we need to or not.
 */
static void
plm_on(int fd, char *addrstr)
{
    insaddr_t i;    
    x10addr_t x;
    char cmd2;

    if (str2insaddr(addrstr, &i)) {
        if (testmode) {
            test_plug = 1;
        } else { 
            do {
                plm_send_insteon(fd, &i, CMD_ON_FAST, 0xff);
            } while (!plm_recv_insteon(fd, &i, NULL, &cmd2, insteon_tmout));
            if (cmd2 == 0)
                err_exit(FALSE, "on command failed");
        }
    } else if (str2x10addr(addrstr, &x)) {
        if (!testmode)
            plm_send_x10(fd, &x, X10_ON);
    } else
        err(FALSE, "could not parse address");
}

/* Send the OFF command to [addrstr], either X10 or Insteon, via PLM on [fd].
 * If Insteon, retry the command until a response is received.
 * If X10, send it x10_attempts times whether we need to or not.
 */
static void
plm_off(int fd, char *addrstr)
{
    insaddr_t i;
    x10addr_t x;
    char cmd2;

    if (str2insaddr(addrstr, &i)) {
        if (testmode) {
            test_plug = 0;
        } else {
            do {
                plm_send_insteon(fd, &i, CMD_OFF_FAST, 0);
            } while (!plm_recv_insteon(fd, &i, NULL, &cmd2, insteon_tmout));
            if (cmd2 != 0)
                err_exit(FALSE, "off command failed");
        }
    } else if (str2x10addr(addrstr, &x)) {
        if (!testmode)
           plm_send_x10(fd, &x, X10_OFF);
    } else
        err(FALSE, "could not parse address");
}

/* Send the Insteon STATUS command to [addrstr] via PLM on [fd] and print 
 * the result on stdout.  Retry the command until a response is received.
 * X10 addresses are accepted here, but we always report status as "unknown".
 */
static void
plm_status(int fd, char *addrstr)
{
    insaddr_t i;
    x10addr_t x;
    char cmd2;

    if (str2insaddr(addrstr, &i)) {
        if (testmode) {
            cmd2 = test_plug;
        } else {
            do {
                plm_send_insteon(fd, &i, CMD_STATUS, 0);
            } while (!plm_recv_insteon(fd, &i, NULL, &cmd2, insteon_tmout));
        }
        printf("%s: %.2hhX\n", addrstr, cmd2);
    } else if (str2x10addr(addrstr, &x))
        printf("%s: unknown\n", addrstr);
    else
        err(FALSE, "could not parse address");
}

/* Send the Insteon PING command to [addrstr] via PLM on [fd] 
 * and print the round-trip time and number of retries on stdout.  
 * Retry the command until a response is received.
 */
static void
plm_ping(int fd, char *addrstr)
{
    insaddr_t i;
    x10addr_t x;
    struct timeval t1, t2, delta;
    int tries = 0;

    /* FIXME: make this work more like ping(8). */

    if (str2insaddr(addrstr, &i)) {
        if (gettimeofday(&t1, NULL) < 0)
            err_exit(TRUE, "gettimeofday");
        do {
            tries++;
            plm_send_insteon(fd, &i, CMD_PING, 0);
        } while (!plm_recv_insteon(fd, &i, NULL, NULL, insteon_tmout));
        if (gettimeofday(&t2, NULL) < 0)
            err_exit(TRUE, "gettimeofday");
        timersub(&t2, &t1, &delta);
        printf("PING from %s: retries=%d time=%.2lf ms\n", addrstr, tries - 1,
               (double)delta.tv_usec / 1000.0 + (double)delta.tv_sec * 1000.0);
    } else if (str2x10addr(addrstr, &x))
        err(FALSE, "ping is only for Insteon devices");
    else
        err(FALSE, "could not parse address");
}

/* Convert a string [s] to Insteon address [ip].
 * Return 1 on success, 0 on failure.
 */
static int 
str2insaddr(char *s, insaddr_t *ip)
{
    if (sscanf(s, "%2hhx.%2hhx.%2hhx", &ip->h, &ip->m, &ip->l) != 3)
        return 0;
    return 1;
}

/* Convert an Insteon address [ip] to string [returned].
 * Returns pointer to static data overwritten on each call.
 */
static char *
insaddr2str(insaddr_t *ip)
{
    static char s[64];

    snprintf(s, sizeof(s), "%.2hhX.%.2hhX.%.2hhX", ip->h, ip->m, ip->l);
    return s;
}

/* Convert a string [s] to X10 address [xp].
 * Return 1 on success, 0 on failure.
 */
static int
str2x10addr(char *s, x10addr_t *xp)
{
    unsigned long i;

    if (isupper(s[0]))
        s[0] = tolower(s[0]);
    if (!islower(s[0]) || s[0] > 'p')
        return 0;
    xp->house = x10_enc[s[0] - 'a'];
    i = strtoul(&s[1], NULL, 10) - 1;
    if (i >= sizeof(x10_enc)/sizeof(int))
        return 0;
    xp->unit = x10_enc[i];
    return 1;
}

/* Wait until PLM on [fd] has data ready for reading or [timeout_msec] expires.
 * Return value is 1 on data ready, 0 on timeout.
 */
static int
wait_until_ready(int fd, int timeout_msec)
{
    struct timeval tv = { timeout_msec / 1000, (timeout_msec % 1000) * 1000 };
    xpollfd_t pfd = xpollfd_create();
    int res = 1;
   
    xpollfd_set(pfd, fd, XPOLLIN);
    if (xpoll(pfd, &tv) == 0)
        res = 0;
    xpollfd_destroy(pfd);

    return res;
}

/* Receive a command or a response from the PLM on [fd] and return it 
 * in [recv] of length [recvlen].  If the a command is received which does 
 * not match [cmd], silently discard it.  If a NAK is received instead of STX
 * indicating the PLM was busy when the last command was sent, return 1, 
 * otherwise return 0.
 */
static int
plm_recv(int fd, char cmd, char *recv, int recvlen)
{
    char b[IM_MAX_RECVLEN];
retry:
    xread_all(fd, &b[0], 1);
    if (b[0] == IM_NAK)
        return 1;
    else if (b[0] != IM_STX)
        err_exit(FALSE, "expected IM_STX or IM_NAK, got %.2hhX", b[0]);
    xread_all(fd, &b[1], 1);
    if (b[1] != cmd) {  
        switch (b[1]) {
            case IM_RECV_STD:
                xread_all(fd, &b[2], 9);
                break;
            case IM_RECV_EXT:
                xread_all(fd, &b[2], 23);
                break;
            case IM_RECV_X10:
                xread_all(fd, &b[2], 2);
                break;
            case IM_RECV_ALL_LINK_COMPLETE:
                xread_all(fd, &b[2], 8);
                break;
            case IM_RECV_BUTTON:
                xread_all(fd, &b[2], 1);
                break;
            case IM_RECV_RESET:
                break;
            case IM_RECV_ALL_LINK_FAIL:
                xread_all(fd, &b[2], 5);
                break;
            case IM_RECV_ALL_LINK_REC:
                xread_all(fd, &b[2], 8);
                break;
            case IM_RECV_ALL_LINK_STAT:
                xread_all(fd, &b[2], 1);
                break;
            default:
                err_exit(FALSE, "unexpected command: %.2hhX", b[1]);
                break;
        }
        goto retry;
    }
    xread_all(fd, &b[2], recvlen - 2);
    memcpy(recv, b, recvlen);
    return 0;
}

/* Send a command [send] of length [sendlen] to PLM on [fd], then
 * read the response into [recv] of length [recvlen].  If a NAK is
 * received, retry the command until it is ACKed.
 */
static void
plm_docmd(int fd, char *send, int sendlen, char *recv, int recvlen)
{
    int nak;

    do {
        xwrite_all(fd, send, sendlen);
        nak = plm_recv(fd, send[1], recv, recvlen);
        if (!nak && recv[recvlen - 1] == IM_NAK) 
            nak = 1; /* FIXME: retry appropriate here or is command broken? */
        if (nak)
            usleep(1000*100); /* wait 100ms for PLM to become ready */
    } while (nak);
}

/* Send IM_RESET to the PLM on [fd].
 */
static void
plm_reset(int fd)
{
    char send[2] = { IM_STX, IM_RESET };
    char recv[3];

    plm_docmd(fd, send, sizeof(send), recv, sizeof(recv));
    printf("PLM reset complete\n");
}

/* Send IM_GET_INFO to the PLM on [fd] and print the response on stdout.
 */
static void
plm_info(int fd)
{
    insaddr_t i;
    char send[2] = { IM_STX, IM_GET_INFO };
    char recv[9];

    plm_docmd(fd, send, sizeof(send), recv, sizeof(recv));
    i.h = recv[2];
    i.m = recv[3];
    i.l = recv[4];
    printf("id=%s dev=%.2hhX.%.2hhX vers=%hhd\n", insaddr2str(&i), 
        recv[5], recv[6], recv[7]);
}

/* Send an Insteon command described by [cmd1] and [cmd2] to address [ip]
 * using PLM on [fd].  The command will be retried until accepted by the PLM.
 */
static void
plm_send_insteon(int fd, insaddr_t *ip, char cmd1, char cmd2)
{
    char send[8] = { IM_STX, IM_SEND, ip->h, ip->m, ip->l, 3, cmd1, cmd2 };
    char recv[9];

    plm_docmd(fd, send, sizeof(send), recv, sizeof(recv));
}

/* Receive an Insteon command from address [i] into [cmd1] and [cmd2]
 * via PLM on [fd].  If more than [timeout_msec] milliseconds elapses,
 * give up and return 0, else return 1.
 */
static int
plm_recv_insteon(int fd, insaddr_t *ip, char *cmd1, char *cmd2, 
                 int timeout_msec)
{
    char recv[11];
    int nak;

    do {
        /* FIXME: timeout is restarted if an unsolicited command is received */
        if (!wait_until_ready(fd, timeout_msec))
            return 0;
        nak = plm_recv(fd, IM_RECV_STD, recv, sizeof(recv));
        if (nak)
            err_exit(FALSE, "unexpected NAK while waiting for Insteon packet");
    } while (ip->h != recv[2] || ip->m != recv[3] || ip->l != recv[4]);
    if (cmd1)
        *cmd1 = recv[9];
    if (cmd2)
        *cmd2 = recv[10];
    return 1;
}

/* Send an X10 command [cmd] to address [xp] via PLM on [fd].  
 * Repeat the command x10_attempts times for good measure because we will
 * get no acknowledgement from the device.
 */
static void
plm_send_x10(int fd, x10addr_t *xp, char cmd)
{
    char senda[4] = { IM_STX, IM_SEND_X10, (xp->house << 4) | xp->unit, 0 };
    char sendb[4] = { IM_STX, IM_SEND_X10, (xp->house << 4) | cmd, 0x80 };
    char recv[5];
    int i;

    for (i = 0; i < x10_attempts; i++) {
        plm_docmd(fd, senda, sizeof(senda), recv, sizeof(recv));
        plm_docmd(fd, sendb, sizeof(sendb), recv, sizeof(recv));
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
