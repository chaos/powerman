/*****************************************************************************\
 *  $Id:$
 *****************************************************************************
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

/* For PLM bits, see 'Insteon Modem Developer's Guide',
 *   http://www.smarthome.com/manuals/2412sdevguide.pdf
 * For general Insteon protocol, see 'Insteon: The Details'
 *   http://www.insteon.net/pdf/insteondetails.pdf
 * For Insteon commands, see 'EZBridge Reference Manual', Appendix B.
 *   http://www.simplehomenet.com/Downloads/EZBridge%20Manual.pdf
 * Also: 'Quick Reference Guide for Smarthome Device Manager for INSTEON'
 *   http://www.insteon.net/sdk/files/dm/docs/
 */

 /* The PLM will send "unsolicited" data (data not sent in response to
  * an action by us) in the following cases, assuming empty all-link db:
  * - Any X10 data appearing on the wire
  * - Direct message with to address matching the IM's insteon ID.
  * - Button press events
  * FIXME: The way plmpower is currently coded, receipt of any of these could
  * interfere with an expected response and cause plmpower to exit.
  * This will be manifested as a spurious command timeout to powerman.
  */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <libgen.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>

#include "xtypes.h"
#include "xmalloc.h"
#include "error.h"
#include "argv.h"
#include "xread.h"

/* PLM commands */
#define IM_STX              0x02
#define IM_INFO             0x60
#define IM_SEND             0x62
#define IM_SEND_X10         0x63
#define IM_RESET            0x67
#define IM_ACK              0x06
#define IM_NAK              0x15
#define IM_RECV_STD         0x50
#define IM_RECV_EXT         0x51
#define IM_RECV_X10         0x52
#define IM_CONFIG_GET       0x73
#define IM_CONFIG_SET       0x6B

/* PLM config bits */
#define IM_CONFIG_BUTLINK_DISABLE  0x80
#define IM_CONFIG_MONITOR_MODE     0x40
#define IM_CONFIG_AUTOLED_DISABLE  0x20
#define IM_CONFIG_DEADMAN_DISABLE  0x10

/* insteon commands */
#define CMD_GRP_ASSIGN      0x01
#define CMD_GRP_DELETE      0x02
#define CMD_PING            0x10
#define CMD_ON              0x11
#define CMD_ON_FAST         0x12
#define CMD_OFF             0x13
#define CMD_OFF_FAST        0x14
#define CMD_BRIGHT          0x15
#define CMD_DIM             0x16
#define CMD_MAN_START       0x17
#define CMD_MAN_STOP        0x18
#define CMD_STATUS          0x19

/* X10 commands */
#define X10_ALL_UNITS_OFF   0x0
#define X10_ALL_LIGHTS_ON   0x1
#define X10_ON              0x2
#define X10_OFF             0x3
#define X10_DIM             0x4
#define X10_BRIGHT          0x5
#define X10_ALL_LIGHTS_OFF  0x6
#define X10_EXT_CODE        0x7
#define X10_HAIL_REQ        0x8
#define X10_HAIL_ACK        0x9
#define X10_PRESET_DIM      0xa
#define X10_PRESET_DIM2     0xb
#define X10_EXT_DATA        0xc
#define X10_STATUS_ON       0xd
#define X10_STATUS_OFF      0xe
#define X10_STATUS_REQ      0xf

/* X10 encoding of housecode A-P and device code 1-16 */
static const int x10_enc[] = { 0x6, 0xe, 0x2, 0xa, 0x1, 0x9, 0x5, 0xd, 
                               0x7, 0xf, 0x3, 0xb, 0x0, 0x8, 0x4, 0xc };

typedef struct {
    char h;
    char m;
    char l;
} addr_t;

typedef struct {
    char house;
    char unit;
} x10addr_t;

#define OPTIONS "d:"
static struct option longopts[] = {
    { "device", required_argument, 0, 'd' },
    {0,0,0,0},
};

static void 
help(void)
{
    printf("Valid commands are:\n");
    printf("  info             get PLM info\n");
    printf("  reset            reset the PLM\n");
    printf("  on     addr      turn on device\n");
    printf("  off    addr      turn on device\n");
    printf("  status addr      query status of device\n");
    printf("  monitor          monitor Insteon/X10 traffic\n");
    printf("Where addr is insteon (xx.xx.xx) or X10 (hdd)\n");
}

static char *
cmd2str(char c)
{
    static char s[16];

    switch (c) {
        case CMD_GRP_ASSIGN:
            strcpy(s, "assign-to-group");
            break;
        case CMD_GRP_DELETE:
            strcpy(s, "delete-from-group");
            break;
        case CMD_PING:
            strcpy(s, "ping");
            break;
        case CMD_ON:
            strcpy(s, "on");
            break;
        case CMD_ON_FAST:
            strcpy(s, "on-fast");
            break;
        case CMD_OFF:
            strcpy(s, "off");
            break;
        case CMD_OFF_FAST:
            strcpy(s, "off-fast");
            break;
        case CMD_BRIGHT:
            strcpy(s, "bright");
            break;
        case CMD_DIM:
            strcpy(s, "dim");
            break;
        case CMD_MAN_START:
            strcpy(s, "start-manual-change");
            break;
        case CMD_MAN_STOP:
            strcpy(s, "stop-manual-change");
            break;
        case CMD_STATUS:
            strcpy(s, "status-request");
            break;
    }
    return s;
}

static int 
str2addr(char *s, addr_t *ap)
{
    if (sscanf(s, "%2hhx.%2hhx.%2hhx", &ap->h, &ap->m, &ap->l) != 3)
        return 0;
    return 1;
}
static char *
addr2str(addr_t *ap)
{
    static char s[64];

    snprintf(s, sizeof(s), "%.2hhX.%.2hhX.%.2hhX", ap->h, ap->m, ap->l);
    return s;
}

static char *
x10cmd2str(char c)
{
    static char s[64];
    switch (c) {
        case X10_ALL_UNITS_OFF:
            strcpy(s, "all-units-off");
            break;
        case X10_ALL_LIGHTS_ON:
            strcpy(s, "all-lights-on");
            break;
        case X10_ON:
            strcpy(s, "on");
            break;
        case X10_OFF:
            strcpy(s, "off");
            break;
        case X10_DIM:
            strcpy(s, "dim");
            break;
        case X10_BRIGHT:
            strcpy(s, "bright");
            break;
        case X10_ALL_LIGHTS_OFF:
            strcpy(s, "all-lights-off");
            break;
        case X10_EXT_CODE:
            strcpy(s, "extended-code");
            break;
        case X10_HAIL_REQ:
            strcpy(s, "hail-request");
            break;
        case X10_HAIL_ACK:
            strcpy(s, "hail-acknowledge");
            break;
        case X10_PRESET_DIM:
        case X10_PRESET_DIM2:
            strcpy(s, "preset-dim");
            break;
        case X10_EXT_DATA:
            strcpy(s, "extended-data");
            break;
        case X10_STATUS_ON:
            strcpy(s, "status=on");
            break;
        case X10_STATUS_OFF:
            strcpy(s, "status=off");
            break;
        case X10_STATUS_REQ:
            strcpy(s, "status-request");
            break;
    }
    return s;
}

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
static char *
x10addr2str(x10addr_t *xp)
{
    static char s[16];
    int i;

    sprintf(s, "???");
    for (i = 0; i < 16; i++)
        if (x10_enc[i] == xp->house)
            s[0] = 'A' + i;
    for (i = 0; i < 16; i++)
        if (x10_enc[i] == xp->unit)
            sprintf(&s[1], "%d", i + 1);
    return s;
}

static char
plm_config_get(int fd)
{
    char send[2] = { IM_STX, IM_CONFIG_GET };
    char recv[6];

    xwrite_all(fd, send, sizeof(send));
    xread_all(fd, recv, sizeof(recv));
    switch (recv[5]) {
        case IM_ACK:
            break;
        case IM_NAK:
        default:
            err_exit(FALSE, "plm_config_get: failed");
    } 
    return recv[2];
}

static void
plm_config_set(int fd, char c)
{
    char send[3] = { IM_STX, IM_CONFIG_SET, c };
    char recv[4];

    xwrite_all(fd, send, sizeof(send));
    xread_all(fd, recv, sizeof(recv));
    switch (recv[3]) {
        case IM_ACK:
            break;
        case IM_NAK:
        default:
            err_exit(FALSE, "plm_config_set: failed");
    } 
}

static void
plm_reset(int fd)
{
    char send[2] = { IM_STX, IM_RESET };
    char recv[3];

    xwrite_all(fd, send, sizeof(send));
    xread_all(fd, recv, sizeof(recv));
    switch (recv[2]) {
        case IM_ACK:
            printf("PLM reset complete\n");
            break;
        case IM_NAK:
        default:
            err_exit(FALSE, "plm_reset: failed");
    } 
}

static void
plm_info(int fd)
{
    addr_t addr;
    char send[2] = { IM_STX, IM_INFO };
    char recv[9];

    xwrite_all(fd, send, sizeof(send));
    xread_all(fd, recv, sizeof(recv));
    switch (recv[8]) {
        case IM_ACK:
            addr.h = recv[2];
            addr.m = recv[3];
            addr.l = recv[4];
            printf("id=%s dev=%.2hhX.%.2hhX vers=%hhd\n", addr2str(&addr), 
                    recv[5], recv[6], recv[7]);
            break;
        case IM_NAK:
        default:
            err_exit(FALSE, "plm_info: failed\n");
    }
}

static void
plm_send(int fd, addr_t *ap, char cmd1, char cmd2)
{
    char send[8] = { IM_STX, IM_SEND, ap->h, ap->m, ap->l, 3, cmd1, cmd2 };
    char recv[9];

    xwrite_all(fd, send, sizeof(send));
    xread_all(fd, recv, sizeof(recv));
    switch (recv[8]) {
        case IM_ACK:
            break;
        default:
        case IM_NAK:
            err_exit(FALSE, "plm_send: failed\n");
    }
}

static void
plm_send_x10(int fd, x10addr_t *xp, char fun)
{
    char send[4] = { IM_STX, IM_SEND_X10, 0, 0 };
    char recv[5];

    send[2] = (xp->house << 4) | xp->unit;
    send[3] = 0;
    xwrite_all(fd, send, sizeof(send));
    xread_all(fd, recv, sizeof(recv));
    switch (recv[4]) {
        case IM_ACK:
            break;
        case IM_NAK:
        default:
            err_exit(FALSE, "plm_send_x10: failed\n");
    }
    sleep(1); /* XXX seems necessary */
    send[2] = (xp->house << 4) | fun;
    send[3] = 0x80;
    xwrite_all(fd, send, sizeof(send));
    xread_all(fd, recv, sizeof(recv));
    switch (recv[4]) {
        case IM_ACK:
            break;
        case IM_NAK:
        default:
            err_exit(FALSE, "plm_send_x10: failed\n");
    }
}

static void
plm_recv(int fd, addr_t *ap, char *cmd1, char *cmd2)
{
    char recv[11];

    xread_all(fd, recv, sizeof(recv));
    if (recv[0] != IM_STX)
        err_exit(FALSE, "plm_recv: unexpected first byte: %.2hhX", recv[0]);
    switch (recv[1]) {
        case IM_RECV_STD:
            if (ap->h != recv[2] || ap->m != recv[3] || ap->l != recv[4])
                err_exit(FALSE, "plm_recv: unexpected from addr: %s",
                        addr2str(ap));
            if (cmd1)
                *cmd1 = recv[9];
            if (cmd2)
                *cmd2 = recv[10];
            break;
        default:
            err_exit(FALSE, "plm_recv: unexpected command: %.2hhX", recv[1]);
    }
}

static void
plm_on(int fd, char *addrstr)
{
    addr_t addr;    
    x10addr_t x;
    char cmd2;

    if (str2addr(addrstr, &addr)) {
        plm_send(fd, &addr, CMD_ON_FAST, 0xff);
        plm_recv(fd, &addr, NULL, &cmd2);
        if (cmd2 == 0)
            err_exit(FALSE, "on command failed");
    } else if (str2x10addr(addrstr, &x))
        plm_send_x10(fd, &x, X10_ON);
    else
        err(FALSE, "could not parse address");
    sleep(1);
}

static void
plm_off(int fd, char *addrstr)
{
    addr_t addr;
    x10addr_t x;
    char cmd2;

    if (str2addr(addrstr, &addr)) {
        plm_send(fd, &addr, CMD_OFF_FAST, 0);
        plm_recv(fd, &addr, NULL, &cmd2);
        if (cmd2 != 0)
            err_exit(FALSE, "off command failed");
    } else if (str2x10addr(addrstr, &x))
       plm_send_x10(fd, &x, X10_OFF);
    else
        err(FALSE, "could not parse address");
    sleep(1);
}

static void
plm_status(int fd, char *addrstr)
{
    addr_t addr;
    x10addr_t x;
    char cmd2;

    if (str2addr(addrstr, &addr)) {
        plm_send(fd, &addr, CMD_STATUS, 0);
        plm_recv(fd, &addr, NULL, &cmd2);
        printf("%s: %.2hhX\n", addrstr, cmd2);
    } else if (str2x10addr(addrstr, &x))
        printf("%s: unknown\n", addrstr);
    else
        err(FALSE, "could not parse address");
}

static void
plm_monitor(int fd)
{
    char c;
    int stx = 0;
    char buf[23];
    addr_t a;
    x10addr_t x;

    /* XXX monitor mode has no effect with an empty all-link db */
    plm_config_set(fd, plm_config_get(fd) | IM_CONFIG_MONITOR_MODE);

    for (;;) {
        xread_all(fd, &c, 1);
        if (!stx) {
            if (c == IM_STX)
                stx = 1;
            else
                printf("Spurious: %.2hhX\n", c);
            continue;
        }
        switch (c) {
            case IM_RECV_STD:   /* untested */
                xread_all(fd, buf, 9);
                a.h = buf[0]; a.m = buf[1]; a.l = buf[2];
                printf("Insteon: [%s -> ", addr2str(&a));
                a.h = buf[3]; a.m = buf[4]; a.l = buf[5];
                printf("%s] %.2hhX %s %.2hhX\n", 
                        addr2str(&a), buf[6], cmd2str(buf[7]), buf[8]);
                stx = 0;
                break;
            case IM_RECV_EXT:   /* untested */
                xread_all(fd, buf, 23);
                a.h = buf[0]; a.m = buf[1]; a.l = buf[2];
                printf("Insteon: [%s -> ", addr2str(&a));
                a.h = buf[3]; a.m = buf[4]; a.l = buf[5];
                printf("%s] %.2hhX %s %.2hhX <extended data>\n", 
                        addr2str(&a), buf[6], cmd2str(buf[7]), buf[8]);
                stx = 0;
                break;
            case IM_RECV_X10:
                xread_all(fd, buf, 2);
                if (buf[1] == 0) {
                    x.house = (buf[0] & 0xf0) >> 4;
                    x.unit = buf[0] & 0x0f;
                    printf("X10: select %s\n", x10addr2str(&x));
                } else if ((unsigned char)buf[1] == 0x80) {
                    printf("X10: %s\n", x10cmd2str(buf[1] & 0x0f));
                } else 
                    printf("X10: <%.2hhX><%.2hhX>\n", buf[2], buf[1]);
                stx = 0;
                break;
            default:
                printf("Unknown PLM command: %.2hhX\n", c);
                stx = 0;
                break;
        }
    }
}

void 
docmd(int fd, char **av, int *quitp)
{
    if (av[0] != NULL) {
        if (strcmp(av[0], "help") == 0) {
            help();
        } else if (strcmp(av[0], "quit") == 0) {
            *quitp = 1;
        } else if (strcmp(av[0], "info") == 0) {
            plm_info(fd);
        } else if (strcmp(av[0], "reset") == 0) {
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
        } else if (strcmp(av[0], "monitor") == 0) {
            plm_monitor(fd);
        } else 
            printf("type \"help\" for a list of commands\n");
    }
}

void shell(int fd)
{
    char buf[128];
    char **av;
    int quit = 0;

    while (!quit) {
        printf("plmpower> ");
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin)) {
            av = argv_create(buf, "");
            docmd(fd, av, &quit);
            argv_destroy(av);
        } else
            quit = 1;
    }
}

void
usage(void)
{
    fprintf(stderr, "Usage: plmpower -d device \n");
    exit(1);
}

static int
open_serial(char *dev)
{
    struct termios tio;
    int fd;

    fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0)
        err_exit(TRUE, "could not open %s", dev);
    if (lockf(fd, F_TLOCK, 0) < 0) /* see comment in device_serial.c */
        err_exit(TRUE, "could not lock %s", dev);
    tcgetattr(fd, &tio);
    tio.c_cflag |= B19200 | CS8 | CLOCAL | CREAD;
    tio.c_iflag = IGNBRK | IGNPAR;
    tio.c_oflag = ONLRET;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 1;
    tcsetattr(fd, TCSANOW, &tio);

    return fd;
}

int
main(int argc, char *argv[])
{
    char *device = NULL;
    int c;
    int fd;

    err_init(basename(argv[0]));

    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'd':
                device = optarg;
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc)
        usage();
    if (device == NULL)
        usage();

    fd = open_serial(device);

    shell(fd);

    if (close(fd) < 0)
        err_exit(TRUE, "error closing %s", device);
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
