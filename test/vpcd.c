/*****************************************************************************
 *  Copyright (C) 2001 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
 *  UCRL-CODE-2002-008.
 *
 *  This file is part of PowerMan, a remote power management program.
 *  For details, see http://code.google.com/p/powerman/
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <libgen.h>
#include <sys/socket.h>
#include <netdb.h>

#include "xmalloc.h"
#include "xread.h"
#include "xpoll.h"

static void usage(void);
static void _noop_handler(int signum);
static void _spew_one(int linenum);
static void _spew(int lines);
static void _prompt_loop(void);
static void _setup_socket(char *port);

#define NUM_PLUGS   16
static int plug[NUM_PLUGS];
static int beacon[NUM_PLUGS];
static int temp[NUM_PLUGS];
static int logged_in = 0;

static char *prog;

#define OPTIONS "p:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"port", required_argument, 0, 'p'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int
main(int argc, char *argv[])
{
    int i, c;
    char *port = NULL;

    prog = basename(argv[0]);

    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
            case 'p':   /* --port n */
                port = xstrdup(optarg);
                break;
            default:
                usage();
        }
    }
    if (optind < argc)
        usage();

    if (signal(SIGPIPE, _noop_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    if (port)
        _setup_socket(port);

    for (i = 0; i < NUM_PLUGS; i++) {
        plug[i] = 0;
        beacon[i] = 0;
        temp[i] = 83 + i;
    }
    _prompt_loop();

    exit(0);
}

static void
usage(void)
{
    fprintf(stderr, "Usage: %s\n", prog);
    exit(1);
}

static void
_noop_handler(int signum)
{
    fprintf(stderr, "%s: received signal %d\n", prog, signum);
}

/* Return with stdin/stdout reopened as a connected socket.
 */
#define LISTEN_BACKLOG 5
static void
_setup_socket(char *serv)
{
    struct addrinfo hints, *res, *r;
    int *fds, fd, fdlen, saved_errno, count, error, i, opt;
    char *what = NULL;
    xpollfd_t pfd;
    struct sockaddr_storage addr;
    socklen_t addr_size;
    short flags;

    /* get addresses to listen on for this port */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((error = getaddrinfo(NULL, serv, &hints, &res))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        exit(1);
    }
    if (res == NULL) {
        fprintf(stderr, "getaddrinfo: no address to bind to\n");
        exit(1);
    }

    /* allocate array of listen fd's */
    fdlen = 0;
    for (r = res; r != NULL; r = r->ai_next)
       fdlen++;
    fds = (int *)xmalloc(sizeof(int) * fdlen);

    /* bind fds to addresses and listen */
    count = 0;
    saved_errno = 0;
    for (r = res, i = 0; r != NULL; r = r->ai_next, i++) {
        fds[i] = -1;
        if ((fd = socket(r->ai_family, r->ai_socktype, 0)) < 0) {
            saved_errno = errno;
            what = "socket";
            continue;
        }
        opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            saved_errno = errno;
            what = "setsockopt";
            close(fd);
            continue;
        }
        if (bind(fd, r->ai_addr, r->ai_addrlen) < 0) {
            saved_errno = errno;
            what = "bind";
            close(fd);
            continue;
        }
        if (listen(fd, LISTEN_BACKLOG) < 0) {
            saved_errno = errno;
            what = "listen";
            close(fd);
            continue;
        }
        fds[i] = fd;
        count++;
    }
    freeaddrinfo(res);
    if (count == 0) {
        fprintf(stderr, "%s: %s\n", what, strerror(saved_errno));
        exit(1);
    }

    /* accept a connection on 'fd' */
    pfd = xpollfd_create();
    fd = -1;
    while (fd == -1) {
        xpollfd_zero(pfd);
        for (i = 0; i < fdlen; i++) {
            if (fds[i] != -1)
                xpollfd_set(pfd, fds[i], XPOLLIN);
        }
        if (xpoll(pfd, NULL) < 0) {
            fprintf(stderr, "poll: %s\n", strerror(errno));
            exit(1);
        }
        for (i = 0; i < fdlen; i++) {
            if (fds[i] != -1) {
                flags = xpollfd_revents(pfd, fds[i]);
                if ((flags & (XPOLLERR|XPOLLHUP|XPOLLNVAL))) {
                        fprintf(stderr, "poll: error on fd %d\n", fds[i]);
                        exit(1);
                }
                if ((flags & XPOLLIN)) {
                    addr_size = sizeof(addr);
                    fd = accept(fds[i], (struct sockaddr *)&addr, &addr_size);
                    if (fd < 0) {
                        fprintf(stderr, "accept: %s\n", strerror(errno));
                        exit(1);
                    }
                    break;
                }
            }
        }
    }
    xpollfd_destroy(pfd);
    for (i = 0; i < fdlen; i++) {
        if (fds[i] != -1 && fds[i] != fd)
            close(fds[i]);
    }

    /* dup socket to stdio */
    (void)close(0);
    if (dup2(fd, 0) < 0) {
        fprintf(stderr, "dup2(stdin): %s\n", strerror(errno));
        exit(1);
    }
    (void)close(1);
    if (dup2(fd, 1) < 0) {
        fprintf(stderr, "dup2(stdout): %s\n", strerror(errno));
        exit(1);
    }
}

#define SPEW \
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]"
static void
_spew_one(int linenum)
{
    char buf[80];

    linenum %= strlen(SPEW);

    memcpy(buf, SPEW + linenum, strlen(SPEW) - linenum);
    memcpy(buf + strlen(SPEW) - linenum, SPEW, linenum);
    buf[strlen(SPEW)] = '\0';
    printf("%s\n", buf);
}

static void
_spew(int lines)
{
    int i;

    for (i = 0; i < lines; i++)
        _spew_one(i);
}

static void
_prompt_loop(void)
{
    int seq, i;
    char buf[128];
    char prompt[16];

    for (seq = 0;; seq++) {
        snprintf(prompt, sizeof(prompt), "%d vpc> ", seq);
        if (xreadline(prompt, buf, sizeof(buf)) == NULL)
            break;
        if (strlen(buf) == 0)
            continue;
        if (strcmp(buf, "logoff") == 0) {               /* logoff */
            printf("%d OK\n", seq);
            logged_in = 0;
            break;
        }
        if (strcmp(buf, "login") == 0) {                /* logon */
            logged_in = 1;
            goto ok;
        }
        if (!logged_in) {
            printf("%d Please login\n", seq);
            continue;
        }
        if (sscanf(buf, "stat %d", &i) == 1) {         /* stat <plugnum> */
            if (i < 0 || i >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, i);
                continue;
            }
            printf("plug %d: %s\n", i, plug[i] ? "ON" : "OFF");
            goto ok;
        }
        if (strcmp(buf, "stat *") == 0) {               /* stat * */
            for (i = 0; i < NUM_PLUGS; i++)
                printf("plug %d: %s\n", i, plug[i] ? "ON" : "OFF");
            goto ok;
        }
        if (sscanf(buf, "beacon %d", &i) == 1) {       /* beacon <plugnum> */
            if (i < 0 || i >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, i);
                continue;
            }
            printf("plug %d: %s\n", i, beacon[i] ? "ON" : "OFF");
            goto ok;
        }
        if (strcmp(buf, "beacon *") == 0) {             /* beacon * */
            for (i = 0; i < NUM_PLUGS; i++)
                printf("plug %d: %s\n", i, beacon[i] ? "ON" : "OFF");
            goto ok;
        }
        if (sscanf(buf, "temp %d", &i) == 1) {          /* temp <plugnum> */
            if (i < 0 || i >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, i);
                continue;
            }
            printf("plug %d: %d\n", i, temp[i]);
        }
        if (strcmp(buf, "temp *") == 0) {               /* temp * */
            for (i = 0; i < NUM_PLUGS; i++)
                printf("plug %d: %d\n", i, temp[i]);
            goto ok;
        }
        if (sscanf(buf, "spew %d", &i) == 1) {          /* spew <linecount> */
            if (i <= 0) {
                printf("%d BADVAL: %d\n", seq, i);
                continue;
            }
            _spew(i);
            goto ok;
        }
        if (sscanf(buf, "on %d", &i) == 1) {           /* on <plugnum> */
            if (i < 0 || i >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, i);
                continue;
            }
            plug[i] = 1;
            goto ok;
        }
        if (sscanf(buf, "off %d", &i) == 1) {          /* off <plugnum> */
            if (i < 0 || i >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, i);
                continue;
            }
            plug[i] = 0;
            goto ok;
        }
        if (sscanf(buf, "flash %d", &i) == 1) {        /* flash <plugnum> */
            if (i < 0 || i >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, i);
                continue;
            }
            beacon[i] = 1;
            goto ok;
        }
        if (sscanf(buf, "unflash %d", &i) == 1) {      /* unflash <plugnum> */
            if (i < 0 || i >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, i);
                continue;
            }
            beacon[i] = 0;
            goto ok;
        }
        if (strcmp(buf, "on *") == 0) {                 /* on * */
            for (i = 0; i < NUM_PLUGS; i++)
                plug[i] = 1;
            goto ok;
        }
        if (strcmp(buf, "off *") == 0) {                /* off * */
            for (i = 0; i < NUM_PLUGS; i++)
                plug[i] = 0;
            goto ok;
        }
        if (sscanf(buf, "reset %d", &i) == 1) {         /* reset <plugnum> */
            if (i < 0 || i >= NUM_PLUGS) {
                printf("%d BADVAL: %d\n", seq, i);
                continue;
            }
            sleep(1); /* noop */
            goto ok;
        }
        if (strcmp(buf, "reset *") == 0) {              /* reset * */
            sleep(1); /* noop */
            goto ok;
        }
        printf("%d UNKNOWN: %s\n", seq, buf);
        continue;
ok:
        printf("%d OK\n", seq);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
