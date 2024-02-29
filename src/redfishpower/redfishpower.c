/************************************************************\
 * Copyright (C) 2021 The Regents of the University of California.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of PowerMan, a remote power management program.
 * For details, see https://github.com/chaos/powerman.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <libgen.h>
#include <curl/curl.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <unistd.h>
#include <sys/select.h>
#include <limits.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "xmalloc.h"
#include "czmq.h"
#include "hostlist.h"
#include "error.h"
#include "argv.h"

static hostlist_t hosts = NULL;
static char *header = NULL;
static struct curl_slist *header_list = NULL;
static int verbose = 0;
static char *userpwd = NULL;
static char *statpath = NULL;
static char *onpath = NULL;
static char *onpostdata = NULL;
static char *offpath = NULL;
static char *offpostdata = NULL;

static int test_mode = 0;
static hostlist_t test_fail_power_cmd_hosts;
static zhashx_t *test_power_status;

/* in seconds */
#define MESSAGE_TIMEOUT            10
#define CMD_TIMEOUT_DEFAULT        60

/* Per documentation, wait incremental time then proceed if timeout < 0 */
#define INCREMENTAL_WAIT           500

/* in usec
 *
 * status polling interval of 1 second may seem long, but testing
 * shows wait ranges from a few seconds to 20 seconds
 */
#define STATUS_POLLING_INTERVAL  1000000

#define MS_IN_SEC                1000

#define CMD_STAT       "stat"
#define CMD_ON         "on"
#define CMD_OFF        "off"

#define STATUS_ON      "on"
#define STATUS_OFF     "off"
#define STATUS_UNKNOWN "unknown"

enum {
      STATE_SEND_POWERCMD,      /* stat, on, off */
      STATE_WAIT_UNTIL_ON_OFF,  /* on, off */
};

struct powermsg {
    CURLM *mh;                  /* curl multi handle pointer */

    CURL *eh;                   /* curl easy handle */
    char *cmd;                  /* "on", "off", or "stat" */
    char *hostname;             /* host we're working with */
    char *url;                  /* on, off, stat */
    char *postdata;             /* on, off */
    char *output;               /* on, off, stat */
    size_t output_len;

    int state;

    /* start - when power op started, may be set to start time of a
     * previous message if this is a follow on message.
     *
     * timeout - when the overall power command times out
     *
     * delaystart - if message should be sent after a wait
     */
    struct timeval start;
    struct timeval timeout;
    struct timeval delaystart;

    /* zlistx handle */
    void *handle;
};

#define Curl_easy_setopt(args)                                                 \
    do {                                                                       \
        CURLcode _ec;                                                          \
        if ((_ec = curl_easy_setopt args) != CURLE_OK)                         \
            err_exit(false, "curl_easy_setopt: %s", curl_easy_strerror(_ec));  \
    } while(0)

#define OPTIONS "h:H:S:O:F:P:G:TEv"
static struct option longopts[] = {
        {"hostname", required_argument, 0, 'h' },
        {"header", required_argument, 0, 'H' },
        {"statpath", required_argument, 0, 'S' },
        {"onpath", required_argument, 0, 'O' },
        {"offpath", required_argument, 0, 'F' },
        {"onpostdata", required_argument, 0, 'P' },
        {"offpostdata", required_argument, 0, 'G' },
        {"test-mode", no_argument, 0, 'T' },
        {"test-fail-power-cmd-hosts", required_argument, 0, 'E' },
        {"verbose", no_argument, 0, 'v' },
        {0,0,0,0},
};

static time_t cmd_timeout = CMD_TIMEOUT_DEFAULT;

void help(void)
{
    printf("Valid commands are:\n");
    printf("  auth user:passwd\n");
    printf("  setheader string\n");
    printf("  setstatpath path\n");
    printf("  setonpath path [postdata]\n");
    printf("  setoffpath path [postdata]\n");
    printf("  settimeout seconds\n");
    printf("  stat [nodes]\n");
    printf("  on [nodes]\n");
    printf("  off [nodes]\n");
}

static size_t output_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct powermsg *pm = userp;

    if (pm->output) {
        char *tmp = calloc(1, pm->output_len + realsize + 1);
        if (!tmp)
            err_exit(true, "calloc");
        memcpy(tmp, pm->output, pm->output_len);
        memcpy(tmp + pm->output_len, contents, realsize);
        pm->output_len += realsize;
        free(pm->output);
        pm->output = tmp;
    }
    else {
        if (!(pm->output = calloc(1, realsize + 1)))
            err_exit(true, "calloc");
        memcpy(pm->output, contents, realsize);
        pm->output_len = realsize;
    }
    return realsize;
}

/* called before putting powermsg on activecmds list */
static void powermsg_init_curl(struct powermsg *pm)
{
    CURLMcode mc;

    if (test_mode)
        return;

    if ((pm->eh = curl_easy_init()) == NULL)
        err_exit(false, "curl_easy_init failed");

    Curl_easy_setopt((pm->eh, CURLOPT_TIMEOUT, MESSAGE_TIMEOUT));
    Curl_easy_setopt((pm->eh, CURLOPT_FAILONERROR, 1));

    /* for time being */
    Curl_easy_setopt((pm->eh, CURLOPT_SSL_VERIFYPEER, 0L));
    Curl_easy_setopt((pm->eh, CURLOPT_SSL_VERIFYHOST, 0L));

    if (verbose > 2)
        Curl_easy_setopt((pm->eh, CURLOPT_VERBOSE, 1L));

    if (header) {
        if (!header_list) {
            if (!(header_list = curl_slist_append(header_list, header)))
                err_exit(false, "curl_slist_append");
        }
        Curl_easy_setopt((pm->eh, CURLOPT_HTTPHEADER, header_list));
    }

    if (userpwd) {
        Curl_easy_setopt((pm->eh, CURLOPT_USERPWD, userpwd));
        Curl_easy_setopt((pm->eh, CURLOPT_HTTPAUTH, CURLAUTH_BASIC));
    }

    Curl_easy_setopt((pm->eh, CURLOPT_WRITEFUNCTION, output_cb));
    Curl_easy_setopt((pm->eh, CURLOPT_WRITEDATA, (void *)pm));

    Curl_easy_setopt((pm->eh, CURLOPT_PRIVATE, pm));

    if ((mc = curl_multi_add_handle(pm->mh, pm->eh)) != CURLM_OK)
        err_exit(false, "curl_multi_add_handle: %s", curl_multi_strerror(mc));

    Curl_easy_setopt((pm->eh, CURLOPT_URL, pm->url));

    if (pm->postdata) {
        Curl_easy_setopt((pm->eh, CURLOPT_POST, 1));
        Curl_easy_setopt((pm->eh, CURLOPT_POSTFIELDS, pm->postdata));
        Curl_easy_setopt((pm->eh, CURLOPT_POSTFIELDSIZE, strlen(pm->postdata)));
    }
    else
        Curl_easy_setopt((pm->eh, CURLOPT_HTTPGET, 1));
}

static struct powermsg *powermsg_create(CURLM *mh,
                                        const char *hostname,
                                        const char *cmd,
                                        const char *path,
                                        const char *postdata,
                                        struct timeval *start,
                                        long int delay_usec,
                                        int state)
{
    struct powermsg *pm = calloc(1, sizeof(*pm));
    struct timeval now;
    struct timeval waitdelay = { 0 };

    if (!pm)
        err_exit(true, "calloc");

    pm->mh = mh;
    pm->state = state;

    pm->cmd = xstrdup(cmd);
    pm->hostname = xstrdup(hostname);

    pm->url = xmalloc(strlen("https://") + strlen(hostname) + strlen(path) + 2);
    sprintf(pm->url, "https://%s/%s", hostname, path);

    if (postdata)
        pm->postdata = xstrdup(postdata);

    if (start) {
        pm->start.tv_sec = start->tv_sec;
        pm->start.tv_usec = start->tv_usec;
    }
    else
        gettimeofday(&pm->start, NULL);

    if (cmd_timeout > (LONG_MAX - pm->start.tv_sec))
        err_exit(false, "cmd_timeout overflow");

    pm->timeout.tv_sec = pm->start.tv_sec + cmd_timeout;
    pm->timeout.tv_usec = pm->start.tv_usec;

    if (delay_usec) {
        gettimeofday(&now, NULL);
        waitdelay.tv_usec = delay_usec;
        timeradd(&now, &waitdelay, &pm->delaystart);
    }
    return pm;
}

static void powermsg_destroy(struct powermsg *pm)
{
    if (pm) {
        if (pm->cmd)
            xfree(pm->cmd);
        if (pm->url)
            xfree(pm->url);
        if (pm->postdata)
            xfree(pm->postdata);
        free(pm->output);
        if (!test_mode && pm->eh) {
            CURLMcode mc;
            Curl_easy_setopt((pm->eh, CURLOPT_URL, ""));
            if ((mc = curl_multi_remove_handle(pm->mh, pm->eh)) != CURLM_OK)
                err_exit(false,
                         "curl_multi_remove_handle: %s",
                         curl_multi_strerror(mc));
            curl_easy_cleanup(pm->eh);
        }
        free(pm);
    }
}

/* get hostlist of input hosts, make sure are legal based on initial input */
static hostlist_t parse_input_hosts(const char *inputhosts)
{
    hostlist_t lhosts = NULL;
    hostlist_iterator_t itr = NULL;
    char *hostname;
    hostlist_t rv = NULL;

    if (!(lhosts = hostlist_create(inputhosts))) {
        printf("illegal hosts input\n");
        return NULL;
    }
    if (!(itr = hostlist_iterator_create(lhosts)))
        err_exit(true, "hostlist_iterator_create");
    while ((hostname = hostlist_next(itr))) {
        if (hostlist_find(hosts, hostname) < 0) {
            printf("unknown host specified: %s\n", hostname);
            free(hostname);
            goto cleanup;
        }
        free(hostname);
    }
    rv = lhosts;
cleanup:
    hostlist_iterator_destroy(itr);
    if (!rv)
        hostlist_destroy(lhosts);
    return rv;
}

static struct powermsg *stat_cmd_host(CURLM * mh, char *hostname)
{
    struct powermsg *pm = powermsg_create(mh,
                                          hostname,
                                          CMD_STAT,
                                          statpath,
                                          NULL,
                                          NULL,
                                          0,
                                          STATE_SEND_POWERCMD);
    return pm;
}

static void stat_cmd(zlistx_t *activecmds, CURLM *mh, char **av)
{
    hostlist_iterator_t itr;
    char *hostname;
    hostlist_t *hostsptr;
    hostlist_t lhosts = NULL;

    if (!statpath) {
        printf("Statpath not setup\n");
        return;
    }

    if (av[0]) {
        if (!(lhosts = parse_input_hosts(av[0])))
            return;
        hostsptr = &lhosts;
    }
    else
        hostsptr = &hosts;

    if (!(itr = hostlist_iterator_create(*hostsptr)))
        err_exit(true, "hostlist_iterator_create");

    while ((hostname = hostlist_next(itr))) {
        struct powermsg *pm = stat_cmd_host(mh, hostname);
        powermsg_init_curl(pm);
        if (!(pm->handle = zlistx_add_end(activecmds, pm)))
            err_exit(true, "zlistx_add_end");
        free(hostname);
    }
    hostlist_iterator_destroy(itr);
    hostlist_destroy(lhosts);
}

static void parse_onoff_response(struct powermsg *pm, const char **strp)
{
    if (pm->output) {
        json_error_t error;
        json_t *o;

        if (!(o = json_loads(pm->output, 0, &error))) {
            (*strp) = "parse error";
            if (verbose)
                printf("%s: parse response error %s\n",
                       pm->hostname,
                       error.text);
        }
        else {
            json_t *val = json_object_get(o, "PowerState");
            if (!val) {
                (*strp) = "no powerstate";
                if (verbose)
                    printf("%s: no PowerState\n", pm->hostname);
            }
            else {
                const char *str = json_string_value(val);
                if (strcasecmp(str, "On") == 0)
                    (*strp) = STATUS_ON;
                else if (strcasecmp(str, "Off") == 0)
                    (*strp) = STATUS_OFF;
                else {
                    (*strp) = STATUS_UNKNOWN;
                    if (verbose)
                        printf("%s: unknown status - %s\n",
                               pm->hostname,str);
                }
            }
        }
        json_decref(o);
    }
    else
        (*strp) = "no output error";
}

static void parse_onoff(struct powermsg *pm, const char **strp)
{
    if (!test_mode) {
        parse_onoff_response(pm, strp);
        return;
    }
    else {
        char *tmp = zhashx_lookup(test_power_status, pm->hostname);
        if (!tmp)
            err_exit(false, "zhashx_lookup on test status failed");
        (*strp) = tmp;
    }
}

static void stat_process(struct powermsg *pm)
{
    const char *str;
    parse_onoff(pm, &str);
    printf("%s: %s\n", pm->hostname, str);
}

static void stat_cleanup(struct powermsg *pm)
{
    powermsg_destroy(pm);
}

struct powermsg *power_cmd_host(CURLM * mh,
                                char *hostname,
                                const char *cmd,
                                const char *path,
                                const char *postdata)
{
    struct powermsg *pm = powermsg_create(mh,
                                          hostname,
                                          cmd,
                                          path,
                                          postdata,
                                          NULL,
                                          0,
                                          STATE_SEND_POWERCMD);
    return pm;
}

static void power_cmd(zlistx_t *activecmds,
                      CURLM *mh,
                      char **av,
                      const char *cmd,
                      const char *path,
                      const char *postdata)
{
    hostlist_iterator_t itr;
    char *hostname;
    hostlist_t *hostsptr;
    hostlist_t lhosts = NULL;

    if (!path) {
        printf("%s path not setup\n", cmd);
        return;
    }

    if (!postdata) {
        printf("%s postdata not setup\n", cmd);
        return;
    }

    if (av[0]) {
        if (!(lhosts = parse_input_hosts(av[0])))
            return;
        hostsptr = &lhosts;
    }
    else
        hostsptr = &hosts;

    if (!(itr = hostlist_iterator_create(*hostsptr)))
        err_exit(true, "hostlist_iterator_create");

    while ((hostname = hostlist_next(itr))) {
        struct powermsg *pm = power_cmd_host(mh, hostname, cmd, path, postdata);
        powermsg_init_curl(pm);
        if (!(pm->handle = zlistx_add_end(activecmds, pm)))
            err_exit(true, "zlistx_add_end");
        free(hostname);
    }
    hostlist_iterator_destroy(itr);
    hostlist_destroy(lhosts);
}

static void on_cmd(zlistx_t *activecmds, CURLM *mh, char **av)
{
    if (!statpath) {
        printf("Statpath not setup\n");
        return;
    }

    power_cmd(activecmds, mh, av, CMD_ON, onpath, onpostdata);
}

static void off_cmd(zlistx_t *activecmds, CURLM *mh, char **av)
{
    if (!statpath) {
        printf("Statpath not setup\n");
        return;
    }

    power_cmd(activecmds, mh, av, CMD_OFF, offpath, offpostdata);
}

static void send_status_poll(zlistx_t *delayedcmds, struct powermsg *pm)
{
    struct powermsg *nextpm;

    /* issue a follow on stat to wait until the on/off is complete.
     * note that we set the initial start time of this new command to
     * the original on/off, so we can timeout correctly
     *
     * in addition, the cmd is the original on/off, indicating the true
     * purpose of this status query
     */
    nextpm = powermsg_create(pm->mh,
                             pm->hostname,
                             pm->cmd,
                             statpath,
                             NULL,
                             &pm->start,
                             STATUS_POLLING_INTERVAL,
                             STATE_WAIT_UNTIL_ON_OFF);
    if (!(nextpm->handle = zlistx_add_end(delayedcmds, nextpm)))
        err_exit(true, "zlistx_add_end");
}

static void on_off_process(zlistx_t *delayedcmds, struct powermsg *pm)
{
    if (pm->state == STATE_SEND_POWERCMD) {
        /* just sent on or off, now we need for the operation to
         * complete
         */
        send_status_poll(delayedcmds, pm);

        /* in test mode, we simulate that the operation has already
         * finished */
        if (test_mode) {
            if (strcmp(pm->cmd, CMD_ON) == 0)
                zhashx_update(test_power_status, pm->hostname, STATUS_ON);
            else /* cmd == CMD_OFF */
                zhashx_update(test_power_status, pm->hostname, STATUS_OFF);
        }
    }
    else if (pm->state == STATE_WAIT_UNTIL_ON_OFF) {
        const char *str;
        struct timeval now;

        parse_onoff(pm, &str);
        if (strcmp(str, pm->cmd) == 0) {
            printf("%s: %s\n", pm->hostname, "ok");
            return;
        }

        /* check if we've timed out */
        gettimeofday(&now, NULL);
        if (timercmp(&now, &pm->timeout, >)) {
            printf("%s: %s\n", pm->hostname, "timeout");
            return;
        }

        /* resend status poll */
        send_status_poll(delayedcmds, pm);
    }

}

static void on_process(zlistx_t *delayedcmds, struct powermsg *pm)
{
    on_off_process(delayedcmds, pm);
}

static void off_process(zlistx_t *delayedcmds, struct powermsg *pm)
{
    on_off_process(delayedcmds, pm);
}

static void power_cmd_process(zlistx_t *delayedcmds, struct powermsg *pm)
{
    if (strcmp(pm->cmd, CMD_STAT) == 0)
        stat_process(pm);
    else if (strcmp(pm->cmd, CMD_ON) == 0)
        on_process(delayedcmds, pm);
    else if (strcmp(pm->cmd, CMD_OFF) == 0)
        off_process(delayedcmds, pm);
}

static void power_cleanup(struct powermsg *pm)
{
    if (!test_mode && pm->eh) {
        Curl_easy_setopt((pm->eh, CURLOPT_POSTFIELDS, ""));
        Curl_easy_setopt((pm->eh, CURLOPT_POSTFIELDSIZE, 0));
    }
    powermsg_destroy(pm);
}

static void on_cleanup(struct powermsg *pm)
{
    power_cleanup(pm);
}

static void off_cleanup(struct powermsg *pm)
{
    power_cleanup(pm);
}

static void auth(char **av)
{
    if (av[0] == NULL) {
        printf("Usage: auth user:passwd\n");
        return;
    }
    if (userpwd)
        xfree(userpwd);
    userpwd = xstrdup(av[0]);
}

static void setheader(char **av)
{
    if (header) {
        xfree(header);
        if (!test_mode)
            curl_slist_free_all(header_list);
        header = NULL;
        header_list = NULL;
    }
    if (av[0]) {
        header = xstrdup(av[0]);
        if (!test_mode)
            header_list = curl_slist_append(header_list, header);
    }
}

static void setstatpath(char **av)
{
    if (statpath) {
        xfree(statpath);
        statpath = NULL;
    }
    if (av[0])
        statpath = xstrdup(av[0]);
}

static void setpowerpath(char **av, char **path, char **postdata)
{
    if (*path) {
        xfree(*path);
        *path = NULL;
    }
    if (*postdata) {
        xfree(*postdata);
        *postdata = NULL;
    }
    if (av[0]) {
        (*path) = xstrdup(av[0]);
        if (av[1])
            (*postdata) = xstrdup(av[1]);
    }
}

static void settimeout(char **av)
{
    if (av[0]) {
        char *endptr;
        long tmp;

        errno = 0;
        tmp = strtol (av[0], &endptr, 10);
        if (errno
            || endptr[0] != '\0'
            || tmp <= 0)
            printf("invalid timeout specified\n");
        cmd_timeout = tmp;
    }
}

static void process_cmd(zlistx_t *activecmds, CURLM *mh, char **av, int *exitflag)
{
    if (av[0] != NULL) {
        if (strcmp(av[0], "help") == 0)
            help();
        else if (strcmp(av[0], "quit") == 0)
            (*exitflag) = 1;
        else if (strcmp(av[0], "auth") == 0)
            auth(av + 1);
        else if (strcmp(av[0], "setheader") == 0)
            setheader(av + 1);
        else if (strcmp(av[0], "setstatpath") == 0)
            setstatpath(av + 1);
        else if (strcmp(av[0], "setonpath") == 0)
            setpowerpath(av + 1, &onpath, &onpostdata);
        else if (strcmp(av[0], "setoffpath") == 0)
            setpowerpath(av + 1, &offpath, &offpostdata);
        else if (strcmp(av[0], "settimeout") == 0)
            settimeout(av + 1);
        else if (strcmp(av[0], CMD_STAT) == 0)
            stat_cmd(activecmds, mh, av + 1);
        else if (strcmp(av[0], CMD_ON) == 0)
            on_cmd(activecmds, mh, av + 1);
        else if (strcmp(av[0], CMD_OFF) == 0)
            off_cmd(activecmds, mh, av + 1);
        else
            printf("type \"help\" for a list of commands\n");
    }
}

static void cleanup_powermsg(void **x)
{
    struct powermsg *pm = *x;
    if (pm) {
        if (strcmp(pm->cmd, CMD_STAT) == 0)
            stat_cleanup(pm);
        else if (strcmp(pm->cmd, CMD_ON) == 0)
            on_cleanup(pm);
        else if (strcmp(pm->cmd, CMD_OFF) == 0)
            off_cleanup(pm);
    }
}

static void shell(CURLM *mh)
{
    zlistx_t *activecmds;
    zlistx_t *delayedcmds;
    int exitflag = 0;

    if (!(activecmds = zlistx_new()))
        err_exit(true, "zlistx_new");
    zlistx_set_destructor(activecmds, cleanup_powermsg);

    if (!(delayedcmds = zlistx_new()))
        err_exit(true, "zlistx_new");
    zlistx_set_destructor(delayedcmds, cleanup_powermsg);

    while (exitflag == 0) {
        CURLMcode mc;
        fd_set fdread;
        fd_set fdwrite;
        fd_set fderror;
        struct timeval timeout = {0};
        struct timeval *timeoutptr = NULL;
        int maxfd = -1;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fderror);

        if (!zlistx_size(activecmds) && !zlistx_size(delayedcmds)) {
            printf("redfishpower> ");
            fflush(stdout);

            FD_SET(STDIN_FILENO, &fdread);
            timeoutptr = NULL;
            maxfd = STDIN_FILENO;
        }
        else {
            struct timeval curl_timeout;
            long curl_timeout_ms;

            /* First check if there are any delayedcmds to send or are
             * waiting.  If there are some ready to send, put to
             * activecmds.  If not, setup timeout accordingly if one
             * is waiting */
            if (zlistx_size(delayedcmds) > 0) {
                struct powermsg *delaypm = zlistx_first(delayedcmds);
                struct timeval delaytimeout;
                struct timeval now;
                gettimeofday(&now, NULL);
                while (delaypm) {
                    if (timercmp(&delaypm->delaystart, &now, >))
                        break;
                    zlistx_detach_cur(delayedcmds);
                    powermsg_init_curl(delaypm);
                    if (!(delaypm->handle = zlistx_add_end(activecmds,
                                                           delaypm)))
                        err_exit(true, "zlistx_add_end");
                    delaypm = zlistx_next(delayedcmds);
                }

                delaypm = zlistx_first(delayedcmds);
                if (delaypm) {
                    timersub(&delaypm->delaystart, &now, &delaytimeout);
                    timeout.tv_sec = delaytimeout.tv_sec;
                    timeout.tv_usec = delaytimeout.tv_usec;
                    timeoutptr = &timeout;
                }
            }

            if (!test_mode) {
                if ((mc = curl_multi_timeout(mh, &curl_timeout_ms)) != CURLM_OK)
                    err_exit(false,
                             "curl_multi_timeout: %s",
                             curl_multi_strerror(mc));
                /* Per documentation, wait incremental time then
                 * proceed if timeout < 0 */
                if (curl_timeout_ms < 0)
                    curl_timeout_ms = INCREMENTAL_WAIT;
                curl_timeout.tv_sec = curl_timeout_ms / MS_IN_SEC;
                curl_timeout.tv_usec = (curl_timeout_ms % MS_IN_SEC) * MS_IN_SEC;

                /* if timeout previously set, must compare */
                if (timeoutptr) {
                    /* only compare if curl_timeout_ms > 0, otherwise
                     * we'd spin
                     */
                    if (curl_timeout_ms > 0) {
                        if (timercmp(&curl_timeout, timeoutptr, <)) {
                            timeoutptr->tv_sec = curl_timeout.tv_sec;
                            timeoutptr->tv_usec = curl_timeout.tv_usec;
                        }
                    }
                }
                else {
                    timeout.tv_sec = curl_timeout.tv_sec;
                    timeout.tv_usec = curl_timeout.tv_usec;
                    timeoutptr = &timeout;
                }

                if ((mc = curl_multi_fdset(mh,
                                           &fdread,
                                           &fdwrite,
                                           &fderror,
                                           &maxfd)) != CURLM_OK)
                    err_exit(false, "curl_multi_fdset: %s", curl_multi_strerror(mc));
            }
            else {
                /* in test-mode assume active cmds complete
                 * "immediately" by setting timeout to 0
                 *
                 * otherwise wait delayedcmds timeout or wait forever
                 * (timeoutptr initialized to NULL)
                 */
                if (zlistx_size(activecmds) > 0) {
                    timeout.tv_sec = 0;
                    timeout.tv_usec = 0;
                    timeoutptr = &timeout;
                }
            }
        }

        /* XXX: use curl_multi_poll/wait on newer versions of curl */

        if (select(maxfd+1, &fdread, &fdwrite, &fderror, timeoutptr) < 0)
            err_exit(true, "select");

        if (FD_ISSET(STDIN_FILENO, &fdread)) {
            char buf[256];
            if (fgets(buf, sizeof(buf), stdin)) {
                char **av;
                av = argv_create(buf, "");
                process_cmd(activecmds, mh, av, &exitflag);
                argv_destroy(av);
            } else
                break;
            if (exitflag)
                break;
        }

        if (zlistx_size(activecmds) == 0)
            continue;

        if (!test_mode) {
            struct CURLMsg *cmsg;
            int msgq = 0;
            int stillrunning;

            if ((mc = curl_multi_perform(mh, &stillrunning)) != CURLM_OK)
                err_exit(false,
                         "curl_multi_perform: %s",
                         curl_multi_strerror(mc));

            do {
                cmsg = curl_multi_info_read(mh, &msgq);
                if(cmsg && (cmsg->msg == CURLMSG_DONE)) {
                    struct powermsg *pm = NULL;
                    CURL *eh = cmsg->easy_handle;
                    CURLcode ec;

                    if ((ec = curl_easy_getinfo(eh,
                                                CURLINFO_PRIVATE,
                                                (char **)&pm)) != CURLE_OK)
                        err_exit(false,
                                 "curl_easy_getinfo: %s",
                                 curl_easy_strerror(ec));

                    if (!pm)
                        err_exit(false, "private data not set in easy handle");

                    if (cmsg->data.result != 0) {
                        if (cmsg->data.result == CURLE_COULDNT_CONNECT
                            || cmsg->data.result == CURLE_OPERATION_TIMEDOUT)
                            printf("%s: %s\n", pm->hostname, "network error");
                        else if (cmsg->data.result == CURLE_HTTP_RETURNED_ERROR) {
                            /* N.B. curl returns this error code for all response
                             * codes >= 400.  So gotta dig in more.
                             */
                            long code;

                            if (curl_easy_getinfo(cmsg->easy_handle,
                                                  CURLINFO_RESPONSE_CODE,
                                                  &code) != CURLE_OK)
                                printf("%s: %s\n", pm->hostname, "http error");
                            if (code == 400)
                                printf("%s: %s\n", pm->hostname, "bad request");
                            else if (code == 401)
                                printf("%s: %s\n", pm->hostname, "unauthorized");
                            else if (code == 404)
                                printf("%s: %s\n", pm->hostname, "not found");
                            else
                                printf("%s: %s (%ld)\n",
                                       pm->hostname,
                                       "http error",
                                       code);
                        }
                        else
                            printf("%s: %s\n", pm->hostname, "error");
                        if (verbose)
                            printf("%s: %s\n", pm->hostname,
                                   curl_easy_strerror(cmsg->data.result));
                    }
                    else
                        power_cmd_process(delayedcmds, pm);
                    fflush(stdout);
                    if (zlistx_delete(activecmds, pm->handle) < 0)
                        err_exit(false, "zlistx_delete failed to delete");
                }
            } while (cmsg);
        }
        else {
            /* in test mode we assume all activecmds complete immediately */
            struct powermsg *pm = zlistx_first(activecmds);
            while (pm) {
                if (hostlist_find(test_fail_power_cmd_hosts, pm->hostname) >= 0)
                    printf("%s: %s\n", pm->hostname, "error");
                else
                    power_cmd_process(delayedcmds, pm);
                fflush(stdout);
                pm = zlistx_next(activecmds);
            }
            zlistx_purge(activecmds);
        }
    }
    zlistx_destroy(&activecmds);
    zlistx_destroy(&delayedcmds);
}

static void usage(void)
{
    fprintf(stderr,
      "Usage: redfishpower --hostname host(s) [OPTIONS]\n"
      "  OPTIONS:\n"
      "  -H, --header        Set extra header string\n"
      "  -S, --statpath      Set stat path\n"
      "  -O, --onpath        Set on path\n"
      "  -F, --offpath       Set off path\n"
      "  -P, --onpostdata    Set on post data\n"
      "  -G, --offpostdata   Set off post data\n"
      "  -v, --verbose       Increase output verbosity\n"
    );
    exit(1);
}

static void init_redfishpower(char *argv[])
{
    err_init(basename(argv[0]));

    if (!(hosts = hostlist_create(NULL)))
        err_exit(true, "hostlist_create error");

    if (!(test_fail_power_cmd_hosts = hostlist_create(NULL)))
        err_exit(true, "hostlist_create error");

    if (!(test_power_status = zhashx_new ()))
        err_exit(false, "zhashx_new error");
}

static void cleanup_redfishpower(void)
{
    xfree(userpwd);
    xfree(statpath);
    xfree(onpath);
    xfree(onpostdata);
    xfree(offpath);
    xfree(offpostdata);

    hostlist_destroy(hosts);

    hostlist_destroy(test_fail_power_cmd_hosts);
    zhashx_destroy(&test_power_status);
}

int main(int argc, char *argv[])
{
    CURLM *mh = NULL;
    CURLcode ec;
    int c;

    init_redfishpower(argv);

    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'h': /* --hostname */
                if (!hostlist_push(hosts, optarg))
                    err_exit(true, "hostlist_push error on %s", optarg);
                break;
            case 'H': /* --header */
                header = xstrdup(optarg);
                break;
            case 'S': /* --statpath */
                statpath = xstrdup(optarg);
                break;
            case 'O': /* --onpath */
                onpath = xstrdup(optarg);
                break;
            case 'F': /* --offpath */
                offpath = xstrdup(optarg);
                break;
            case 'P': /* --onpostdata */
                onpostdata = xstrdup(optarg);
                break;
            case 'G': /* --offpostdata */
                offpostdata = xstrdup(optarg);
                break;
            case 'T': /* --test-mode */
                test_mode = 1;
                break;
            case 'E': /* --test-fail-power-cmd-hosts */
                if (!hostlist_push(test_fail_power_cmd_hosts, optarg))
                    err_exit(true, "hostlist_push error on %s", optarg);
                break;
            case 'v': /* --verbose */
                verbose++;
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc)
        usage();

    if (hostlist_count(hosts) == 0)
        usage();

    if (!test_mode) {
        if ((ec = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK)
            err_exit(false, "curl_global_init: %s", curl_easy_strerror(ec));

        if (!(mh = curl_multi_init()))
            err_exit(false, "curl_multi_init failed");
    }
    else {
        /* All hosts initially are off for testing */
        hostlist_iterator_t itr;
        char *hostname;
        if (!(itr = hostlist_iterator_create(hosts)))
            err_exit(true, "hostlist_iterator_create");
        while ((hostname = hostlist_next(itr))) {
            if (zhashx_insert(test_power_status, hostname, STATUS_OFF) < 0)
                err_exit(false, "zhash_insert failure");
            free(hostname);
        }
        hostlist_iterator_destroy(itr);
    }

    shell(mh);

    if (!test_mode)
        curl_multi_cleanup(mh);

    cleanup_redfishpower();
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
