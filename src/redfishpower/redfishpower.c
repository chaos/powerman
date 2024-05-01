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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include "redfishpower_defs.h"
#include "plugs.h"

#include "xmalloc.h"
#include "czmq.h"
#include "hostlist.h"
#include "error.h"
#include "argv.h"

static hostlist_t hosts = NULL;
static plugs_t *plugs = NULL;
/* flag to indicate if we wiped initial plugs */
static int initial_plugs_setup = 0;
static char *header = NULL;
static struct curl_slist *header_list = NULL;
static int resolve_hosts = 0;
static int verbose = 0;
static char *userpwd = NULL;
static int userpwd_set_on_cmdline = 0;

/* default paths if host specific ones not set */
static char *statpath = NULL;
static char *onpath = NULL;
static char *onpostdata = NULL;
static char *offpath = NULL;
static char *offpostdata = NULL;

/* activecmds - power ops to be sent / in progress now */
static zlistx_t *activecmds = NULL;
/* delayedcmds - power ops waiting to be sent
 * - typically holds status polling ops after an on / off, we wait to
 *   send at a later time.
 */
static zlistx_t *delayedcmds = NULL;
/* waitcmds - power ops waiting for a parent check to be completed */
static zlistx_t *waitcmds = NULL;

static int test_mode = 0;
static hostlist_t test_fail_power_cmd_hosts;
static zhashx_t *test_power_status;

static zhashx_t *resolve_hosts_cache = NULL;

/* in seconds */
#define MESSAGE_TIMEOUT_DEFAULT    5
#define CMD_TIMEOUT_DEFAULT        60

/* Per documentation, wait incremental time then proceed if timeout < 0 */
#define INCREMENTAL_WAIT           500

/* in usec */
#define STATUS_POLLING_INTERVAL_DEFAULT  1000000

#define MS_IN_SEC                1000

#define STATUS_ON           "on"
#define STATUS_OFF          "off"
#define STATUS_PAUSED       "Paused"
#define STATUS_POWERING_ON  "PoweringOn"
#define STATUS_POWERING_OFF "PoweringOff"
#define STATUS_UNKNOWN      "unknown"
#define STATUS_ERROR        "error"

#define OUTPUT_RESULT  1
#define NO_OUTPUT      0

/* achu: max length IPv6 is 45 chars, add +1 for NUL
 * ABCD:ABCD:ABCD:ABCD:ABCD:ABCD:192.168.100.200
 */
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

enum {
      STATE_SEND_POWERCMD,      /* stat, on, off */
      STATE_WAIT_UNTIL_ON_OFF,  /* on, off */
};

struct powermsg {
    CURLM *mh;                  /* curl multi handle pointer */

    CURL *eh;                   /* curl easy handle */
    char *cmd;                  /* "on", "off", or "stat" */
    char *hostname;             /* host we're working with */
    char *plugname;             /* plugname, in some cases == hostname */
    char *parent;               /* parent, NULL if no parent */
    char *url;                  /* on, off, stat */
    char *postdata;             /* on, off */
    char *output;               /* on, off, stat */
    size_t output_len;

    int output_result;          /* output result or not */

    int state;

    /* start - when power op started, may be set to start time of a
     * previous message if this is a follow on message.
     *
     * timeout - when the overall power command times out
     *
     * delaystart - if message should be sent after a wait
     *
     * poll_count - number of poll attempts
     */
    struct timeval start;
    struct timeval timeout;
    struct timeval delaystart;
    int poll_count;

    /* zlistx handle */
    void *handle;
};

#define Curl_easy_setopt(args)                                                 \
    do {                                                                       \
        CURLcode _ec;                                                          \
        if ((_ec = curl_easy_setopt args) != CURLE_OK)                         \
            err_exit(false, "curl_easy_setopt: %s", curl_easy_strerror(_ec));  \
    } while(0)

#define OPTIONS "h:A:H:S:O:F:P:G:m:TEv"
static struct option longopts[] = {
        {"hostname", required_argument, 0, 'h' },
        {"header", required_argument, 0, 'H' },
        {"auth", required_argument, 0, 'A' },
        {"statpath", required_argument, 0, 'S' },
        {"onpath", required_argument, 0, 'O' },
        {"offpath", required_argument, 0, 'F' },
        {"onpostdata", required_argument, 0, 'P' },
        {"offpostdata", required_argument, 0, 'G' },
        {"message-timeout", required_argument, 0, 'm' },
        {"resolve-hosts", no_argument, 0, 'o' },
        {"test-mode", no_argument, 0, 'T' },
        {"test-fail-power-cmd-hosts", required_argument, 0, 'E' },
        {"verbose", no_argument, 0, 'v' },
        {0,0,0,0},
};

static time_t cmd_timeout = CMD_TIMEOUT_DEFAULT;
/* typically is of type suseconds_t, but has questionable portability,
 * so use 'long int' instead
 */
static long int status_polling_interval = STATUS_POLLING_INTERVAL_DEFAULT;
static long message_timeout = MESSAGE_TIMEOUT_DEFAULT;

void help(void)
{
    printf("Valid commands are:\n");
    printf("  auth user:passwd\n");
    printf("  setheader string\n");
    printf("  setstatpath path\n");
    printf("  setonpath path [postdata]\n");
    printf("  setoffpath path [postdata]\n");
    printf("  setplugs plugnames hostindices [<parentplug]]\n");
    printf("  setpath plugnames cmd path [postdata]\n");
    printf("  settimeout seconds\n");
    printf("  stat [plugs]\n");
    printf("  on [plugs]\n");
    printf("  off [plugs]\n");
}

static char *calc_path(const char *lpath, const char *plugname)
{
    char *ptr;

    /* assumption, at most one {{plug}} in path */
    if ((ptr = strstr(lpath, "{{plug}}"))) {
        /* +1 for NUL, -8 for {{plug}}, + strlen(plugname) */
        char *tmp = xmalloc(strlen(lpath) + 1 - 8 + strlen(plugname));
        int prefixlen = ptr - lpath;
        strncpy(tmp, lpath, prefixlen);
        strcpy(tmp + prefixlen, plugname);
        strcpy(tmp + prefixlen + strlen(plugname), ptr + 8);
        return tmp;
    }
    return xstrdup(lpath);
}

static void get_path(const char *cmd,
                     const char *plugname,
                     char **path,
                     char **postdata)
{
    struct plug_data *pd = plugs_get_data(plugs, plugname);
    char *lpath = NULL;
    char *lpostdata = NULL;

    if (strcmp(cmd, CMD_STAT) == 0) {
        if (pd && pd->stat)
            lpath = pd->stat;
        else
            lpath = statpath;
    }
    else if (strcmp(cmd, CMD_ON) == 0) {
        if (pd && pd->on) {
            lpath = pd->on;
            lpostdata = pd->onpostdata;
        }
        else {
            lpath = onpath;
            lpostdata = onpostdata;
        }
    }
    else if (strcmp(cmd, CMD_OFF) == 0) {
        if (pd && pd->off) {
            lpath = pd->off;
            lpostdata = pd->offpostdata;
        }
        else {
            lpath = offpath;
            lpostdata = offpostdata;
        }
    }

    if (lpath)
        (*path) = calc_path(lpath, plugname);
    if (lpostdata)
        (*postdata) = xstrdup(lpostdata);
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

    /* Per documentation, CURLOPT_TIMEOUT overrides
     * CURLOPT_CONNECTTIMEOUT */
    Curl_easy_setopt((pm->eh, CURLOPT_TIMEOUT, message_timeout));
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

static char *resolve_hosts_url(const char *hostname, const char *path)
{
    char *url;
    char ipstr[INET6_ADDRSTRLEN] = {0};
    size_t len;
    struct addrinfo *ai;
    struct addrinfo *res = NULL;
    int ret;
    char *ptr;

    if ((ptr = zhashx_lookup(resolve_hosts_cache, hostname))) {
        len = strlen("https://") + strlen(ptr) + strlen(path) + 2;
        url = xmalloc(len);
        sprintf(url, "https://%s/%s", ptr, path);
        return url;
    }

    if ((ret = getaddrinfo(hostname, NULL, NULL, &res)))
        err_exit(false, "getaddrinfo: %s", gai_strerror (ret));

    for (ai = res; ai != NULL; ai = ai->ai_next) {
        if (ai->ai_family == AF_INET) {
            struct sockaddr_in addr;

            memcpy(&addr, ai->ai_addr, ai->ai_addrlen);

            if (!inet_ntop (AF_INET,
                            &addr.sin_addr,
                            ipstr,
                            INET6_ADDRSTRLEN))
                err_exit(true, "inet_ntop");

            if (zhashx_insert(resolve_hosts_cache, hostname, xstrdup(ipstr)) < 0)
                err_exit(false, "zhashx_insert");

            len = strlen("https://") + strlen(ipstr) + strlen(path) + 2;
            url = xmalloc(len);
            sprintf(url, "https://%s/%s", ipstr, path);
            goto out;
        }
        else if (ai->ai_family == AF_INET6) {
            struct sockaddr_in6 addr6;

            memcpy(&addr6, ai->ai_addr, ai->ai_addrlen);

            if (!inet_ntop (AF_INET6,
                            &addr6.sin6_addr,
                            ipstr,
                            INET6_ADDRSTRLEN))
                err_exit(true, "inet_ntop");

            if (zhashx_insert(resolve_hosts_cache, hostname, xstrdup(ipstr)) < 0)
                err_exit(false, "zhashx_insert");

            len = strlen("https://") + strlen(ipstr) + strlen(path) + 2;
            url = xmalloc(len);
            sprintf(url, "https://%s/%s", ipstr, path);
            goto out;
        }
    }

    /* couldn't find? use host */
    url = xmalloc(strlen("https://") + strlen(hostname) + strlen(path) + 2);
    sprintf(url, "https://%s/%s", hostname, path);
out:
    freeaddrinfo(res);
    return url;
}

static struct powermsg *powermsg_create(CURLM *mh,
                                        const char *hostname,
                                        const char *plugname,
                                        const char *parent,
                                        const char *cmd,
                                        const char *path,
                                        const char *postdata,
                                        struct timeval *start,
                                        long int delay_usec,
                                        int poll_count,
                                        int output_result,
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
    pm->plugname = xstrdup(plugname);
    if (parent)
        pm->parent = xstrdup(parent);

    pm->output_result = output_result;

    if (resolve_hosts)
        pm->url = resolve_hosts_url(hostname, path);
    else {
        pm->url = xmalloc(strlen("https://") + strlen(hostname) + strlen(path) + 2);
        sprintf(pm->url, "https://%s/%s", hostname, path);
    }

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

    pm->poll_count = poll_count;

    return pm;
}

static void powermsg_destroy(struct powermsg *pm)
{
    if (pm) {
        xfree(pm->cmd);
        xfree(pm->hostname);
        xfree(pm->plugname);
        xfree(pm->parent);
        xfree(pm->url);
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

static struct powermsg *stat_cmd_plug(CURLM * mh,
                                      char *plugname,
                                      int output_result)
{
    struct powermsg *pm;
    struct plug_data *pd;
    char *path = NULL;

    if (!(pd = plugs_get_data(plugs, plugname))) {
        printf("plug not mapped: %s\n", plugname);
        return NULL;
    }

    get_path(CMD_STAT, plugname, &path, NULL);
    if (!path) {
        printf("%s: %s path not set\n", plugname, CMD_STAT);
        return NULL;
    }

    pm = powermsg_create(mh,
                         pd->hostname,
                         plugname,
                         pd->parent,
                         CMD_STAT,
                         path,
                         NULL,
                         NULL,
                         0,
                         0,
                         output_result,
                         STATE_SEND_POWERCMD);
    if (verbose > 1)
        printf("DEBUG: %s hostname=%s plugname=%s path=%s\n",
               CMD_STAT, pd->hostname, plugname, path);
    free(path);
    return pm;
}

/* is parent plugname already active?
 * - if command is "on"/"off"/"stat" and plugname command is "stat',
 *   counts as active
 * - if command is "off" and plugname being turned off, counts as active
 *
 * - note, we do not support command "on" and plugname being turned on is "on.
 * see notes in phased_power_on_check().
 */
static int plugname_active(const char *plugname, const char *cmd)
{
    struct powermsg *pm = zlistx_first(activecmds);
    while (pm) {
        if (strcmp(pm->plugname, plugname) == 0) {
            if (strcmp(pm->cmd, CMD_STAT) == 0)
                return 1;
            else if (strcmp(cmd, CMD_OFF) == 0
                     && strcmp(pm->cmd, CMD_OFF) == 0)
                return 1;
        }
        pm = zlistx_next(activecmds);
    }
    return 0;
}

static void send_initial_parent_queries(CURLM *mh)
{
    /* Only send out one query for identical ancestors */
    struct powermsg *pm = zlistx_first(waitcmds);
    while (pm) {
        char *root_plugname = plugs_find_root_parent(plugs, pm->plugname);
        assert(root_plugname);
        int is_active = plugname_active(root_plugname, pm->cmd);
        /* if not active, that means no active attempts to on/off/stat
         * the root plugname, so we need to stat it now
         */
        if (!is_active) {
            struct powermsg *rootpm;
            /* no_output, we do not output this query result since it
             * is only to determine if we should perform power op
             */
            rootpm = stat_cmd_plug(mh, root_plugname, NO_OUTPUT);
            if (!rootpm)
                goto next;
            powermsg_init_curl(rootpm);
            if (!(rootpm->handle = zlistx_add_end(activecmds, rootpm)))
                err_exit(true, "zlistx_add_end");
            if (verbose > 1)
                fprintf(stderr,
                        "DEBUG: parent query hostname=%s plugname=%s\n",
                        rootpm->hostname, rootpm->plugname);
        }
    next:
        pm = zlistx_next(waitcmds);
    }
}

static void stat_cmd(CURLM *mh, char **av)
{
    hostlist_iterator_t itr;
    char *plugname;
    hostlist_t *plugsptr;
    hostlist_t lplugs = NULL;

    if (av[0]) {
        if (!(lplugs = hostlist_create(av[0]))) {
            printf("illegal hosts input\n");
            return;
        }
        plugsptr = &lplugs;
    }
    else
        plugsptr = plugs_hostlist(plugs);

    if (!(itr = hostlist_iterator_create(*plugsptr)))
        err_exit(true, "hostlist_iterator_create");

    while ((plugname = hostlist_next(itr))) {
        struct powermsg *pm;
        if (!plugs_name_valid(plugs, plugname)) {
            printf("unknown plug specified: %s\n", plugname);
            free(plugname);
            continue;
        }
        if (!(pm = stat_cmd_plug(mh, plugname, OUTPUT_RESULT))) {
            free(plugname);
            continue;
        }
        if (pm->parent) {
            if (!(pm->handle = zlistx_add_end(waitcmds, pm)))
                err_exit(true, "zlistx_add_end");
        }
        else {
            powermsg_init_curl(pm);
            if (!(pm->handle = zlistx_add_end(activecmds, pm)))
                err_exit(true, "zlistx_add_end");
        }
        free(plugname);
    }

    if (zlistx_size(waitcmds) > 0)
        send_initial_parent_queries(mh);

    hostlist_iterator_destroy(itr);
    hostlist_destroy(lplugs);
}

/* status_strp - on, off, unknown
 * rstatus_strp - on, off, paused, poweringoff, poweringon, unknown
 */
static void parse_onoff_response(struct powermsg *pm,
                                 const char **status_strp,
                                 const char **rstatus_strp)
{
    if (pm->output) {
        json_error_t error;
        json_t *o;

        if (!(o = json_loads(pm->output, 0, &error))) {
            (*status_strp) = "parse error";
            if (rstatus_strp)
                (*rstatus_strp) = "parse error";
            if (verbose)
                printf("%s: parse response error %s\n",
                       pm->plugname,
                       error.text);
        }
        else {
            json_t *val = json_object_get(o, "PowerState");
            if (!val) {
                (*status_strp) = "no powerstate";
                if (verbose)
                    printf("%s: no PowerState\n", pm->plugname);
            }
            else {
                const char *str = json_string_value(val);
                if (strcasecmp(str, "On") == 0) {
                    (*status_strp) = STATUS_ON;
                    if (rstatus_strp)
                        (*rstatus_strp) = STATUS_ON;
                }
                else if (strcasecmp(str, "Off") == 0) {
                    (*status_strp) = STATUS_OFF;
                    if (rstatus_strp)
                        (*rstatus_strp) = STATUS_OFF;
                }
                else {
                    (*status_strp) = STATUS_UNKNOWN;
                    if (rstatus_strp) {
                        if (strcasecmp(str, "Paused") == 0)
                            (*rstatus_strp) = STATUS_PAUSED;
                        else if (strcasecmp(str, "PoweringOff") == 0)
                            (*rstatus_strp) = STATUS_POWERING_OFF;
                        else if (strcasecmp(str, "PoweringOn") == 0)
                            (*rstatus_strp) = STATUS_POWERING_ON;
                        else
                            (*rstatus_strp) = STATUS_UNKNOWN;
                    }
                    if (verbose)
                        printf("%s: unknown status - %s\n",
                               pm->plugname, str);
                }
            }
        }
        json_decref(o);
    }
    else
        (*status_strp) = "no output error";
}

static void parse_onoff(struct powermsg *pm,
                        const char **status_strp,
                        const char **rstatus_strp)
{
    if (!test_mode) {
        parse_onoff_response(pm, status_strp, rstatus_strp);
        return;
    }
    else {
        char *tmp = zhashx_lookup(test_power_status, pm->plugname);
        if (!tmp)
            err_exit(false, "zhashx_lookup on test status failed");
        (*status_strp) = tmp;
        if (rstatus_strp)
            (*rstatus_strp) = tmp;
    }
}

static void process_waiters(CURLM *mh,
                            const char *ancestor,
                            const char *status_str)
{
    struct powermsg *pm;

    /* first pass - deal with descendants that will be removed from
     * waitcmds (either removed outright or moved to activecmds)
     */
    pm = zlistx_first(waitcmds);
    while (pm) {
        int descendant = plugs_is_descendant(plugs, pm->plugname, ancestor);
        if (descendant) {
            if (strcmp(status_str, STATUS_ON) != 0) {
                if (verbose > 1)
                    fprintf(stderr,
                            "DEBUG: descendant: "
                            "%s hostname=%s plugname=%s status=%s\n",
                            pm->cmd, pm->hostname, pm->plugname, status_str);

                if (pm->output_result) {
                    /* for stat if ancestor is off/unknown/error, the child is
                     * defined as off/unknown/error
                     *
                     * for off, if ancestor is off, we consider
                     * operation a success
                     *
                     * otherwise can't do operation
                     */
                    if (strcmp(pm->cmd, CMD_STAT) == 0)
                        printf("%s: %s\n", pm->plugname, status_str);
                    else if (strcmp(pm->cmd, CMD_OFF) == 0
                             && strcmp(status_str, STATUS_OFF) == 0)
                        printf("%s: %s\n", pm->plugname, "ok");
                    else {
                        struct plug_data *pd = plugs_get_data(plugs, ancestor);
                        printf("%s: cannot perform %s, dependency %s"
                               " (host=%s plug=%s)\n",
                               pm->plugname,
                               pm->cmd,
                               status_str,
                               pd->hostname,
                               pd->plugname);
                    }
                }

                /* this power op is now done */
                zlistx_detach_cur(waitcmds);
                powermsg_destroy(pm);
            }
            else {
                /* if ancestor is direct parent, move waiter to active list */
                if (strcmp(pm->parent, ancestor) == 0) {
                    if (verbose > 1)
                        fprintf(stderr,
                                "DEBUG: %s hostname=%s plugname=%s "
                                "moved to activecmds\n",
                                pm->cmd, pm->hostname, pm->plugname);
                    zlistx_detach_cur(waitcmds);
                    powermsg_init_curl(pm);
                    if (!(pm->handle = zlistx_add_end(activecmds, pm)))
                        err_exit(true, "zlistx_add_end");
                }
            }
        }
        pm = zlistx_next(waitcmds);
    }

    /* second loop only deals w/ STATUS_ON, so we can give up now if
     * not on */
    if (strcmp(status_str, STATUS_ON) != 0)
        return;

    /* second pass - remaining descendants on waitcmds do not have
     * direct parent == ancestor, so we have to status query
     * the child of that ancestor.
     *
     * This has to be done as a second pass because we need to
     * previous loop to finish moving all possible power ops on
     * waitcmds to the activecmds list before we scan it with
     * plugname_active().
     */
    pm = zlistx_first(waitcmds);
    while (pm) {
        int descendant = plugs_is_descendant(plugs, pm->plugname, ancestor);
        if (descendant) {
            /* status query the child of that ancestor */
            char *child = plugs_child_of_ancestor(plugs, pm->plugname, ancestor);
            assert(child);
            int is_active = plugname_active(child, pm->cmd);
            /* if not active, that means no active attempts to on/off/stat
             * the child plugname, so we need to stat it now
             */
            if (!is_active) {
                struct powermsg *childpm;
                /* no_output, we do not output this query result since it
                 * is only to determine if we should perform power op
                 */
                childpm = stat_cmd_plug(mh, child, NO_OUTPUT);
                if (!childpm)
                    goto next;
                powermsg_init_curl(childpm);
                if (!(childpm->handle = zlistx_add_end(activecmds, childpm)))
                    err_exit(true, "zlistx_add_end");
                if (verbose > 1)
                    fprintf(stderr,
                            "DEBUG: parent query hostname=%s plugname=%s\n",
                            childpm->hostname, childpm->plugname);
            }
        }
    next:
        pm = zlistx_next(waitcmds);
    }
}

static void stat_process(struct powermsg *pm)
{
    const char *status_str;
    parse_onoff(pm, &status_str, NULL);
    if (pm->output_result)
        printf("%s: %s\n", pm->plugname, status_str);
    if (verbose > 1)
        fprintf(stderr,
                "DEBUG: %s hostname=%s plugname=%s status=%s\n",
                pm->cmd, pm->hostname, pm->plugname, status_str);

    process_waiters(pm->mh, pm->plugname, status_str);
}

static void stat_cleanup(struct powermsg *pm)
{
    powermsg_destroy(pm);
}

struct powermsg *power_cmd_plug(CURLM * mh,
                                char *plugname,
                                const char *cmd)
{
    struct powermsg *pm;
    struct plug_data *pd;
    char *path = NULL;
    char *postdata = NULL;

    if (!(pd = plugs_get_data(plugs, plugname))) {
        printf("plug not mapped: %s\n", plugname);
        return NULL;
    }

    get_path(cmd, plugname, &path, &postdata);
    if (!path) {
        printf("%s: %s path not set\n", plugname, cmd);
        return NULL;
    }

    pm = powermsg_create(mh,
                         pd->hostname,
                         plugname,
                         pd->parent,
                         cmd,
                         path,
                         postdata,
                         NULL,
                         0,
                         0,
                         OUTPUT_RESULT,
                         STATE_SEND_POWERCMD);
    if (verbose > 1)
        printf("DEBUG: %s hostname=%s plugname=%s path=%s\n",
               cmd, pd->hostname, plugname, path);
    free(path);
    free(postdata);
    return pm;
}

/* We do not allow "phased" power on, where we power on a
 * parent, wait for it to finish, then power on a child.
 *
 * It ultimately proved too difficult to be done consistently well.
 * In many cases, you cannot power on a child after a parent.  There
 * must be a delay as the parent "sets itself up".  The delay can be
 * unknown.  Testing on some hardware has shown itself to last
 * multiple minutes.
 *
 * Hypothetically, we could try powering on a child multiple times
 * until we succeed or some global timeout is reached.  However, this
 * proves difficult, as there are varying degrees of errors that are
 * returned from that power on and it is difficult to ascertain which
 * errors are "good" vs "bad".  e.g. is a connection timeout permanent
 * b/c a child is missing?  or is a connection timeout temporary?
 *
 * So we simply don't allow it.  Powering on different levels
 * is left to users.
 */
static void phased_power_on_check(const char *cmd)
{
    struct powermsg **allpm;
    struct powermsg *pm;
    int total = 0;
    int i = 0;
    int cancel = 0;

    if (strcmp(cmd, CMD_ON) != 0)
        return;

    total += zlistx_size(activecmds);
    total += zlistx_size(waitcmds);

    assert(total > 0);

    /* single target can't be an ancestor of anyone else */
    if (total == 1)
        return;

    /* we need to compare every target to every other target, an array
     * is easier for this than the lists.
     */

    allpm = (struct powermsg **)xmalloc(sizeof(*allpm) * total);

    pm = zlistx_first(activecmds);
    while (pm) {
        allpm[i++] = pm;
        pm = zlistx_next(activecmds);
    }

    pm = zlistx_first(waitcmds);
    while (pm) {
        allpm[i++] = pm;
        pm = zlistx_next(waitcmds);
    }

    for (i = 0; i < (total - 1); i++) {
        struct powermsg *pm1 = allpm[i];
        for (int j = i + 1; j < total; j++) {
            struct powermsg *pm2 = allpm[j];
            int d1 = plugs_is_descendant(plugs, pm1->plugname, pm2->plugname);
            int d2 = plugs_is_descendant(plugs, pm2->plugname, pm1->plugname);
            if (d1 || d2) {
                cancel++;
                break;
            }
        }
    }

    if (cancel) {
        pm = zlistx_first(activecmds);
        while (pm) {
            printf("%s: %s\n", pm->plugname, "cannot turn on parent and child");
            pm = zlistx_next(activecmds);
        }
        zlistx_purge(activecmds);

        pm = zlistx_first(waitcmds);
        while (pm) {
            printf("%s: %s\n", pm->plugname, "cannot turn on parent and child");
            pm = zlistx_next(waitcmds);
        }
        zlistx_purge(waitcmds);
    }

    free(allpm);
    return;
}

static void power_cmd(CURLM *mh,
                      char **av,
                      const char *cmd)
{
    hostlist_iterator_t itr;
    char *plugname;
    hostlist_t *plugsptr;
    hostlist_t lplugs = NULL;

    if (av[0]) {
        if (!(lplugs = hostlist_create(av[0]))) {
            printf("illegal hosts input\n");
            return;
        }
        plugsptr = &lplugs;
    }
    else
        plugsptr = plugs_hostlist(plugs);

    if (!(itr = hostlist_iterator_create(*plugsptr)))
        err_exit(true, "hostlist_iterator_create");

    while ((plugname = hostlist_next(itr))) {
        struct powermsg *pm;
        if (!plugs_name_valid(plugs, plugname)) {
            printf("unknown plug specified: %s\n", plugname);
            free(plugname);
            continue;
        }
        if (!(pm = power_cmd_plug(mh, plugname, cmd))) {
            free(plugname);
            continue;
        }
        if (pm->parent) {
            if (!(pm->handle = zlistx_add_end(waitcmds, pm)))
                err_exit(true, "zlistx_add_end");
        }
        else {
            powermsg_init_curl(pm);
            if (!(pm->handle = zlistx_add_end(activecmds, pm)))
                err_exit(true, "zlistx_add_end");
        }
        free(plugname);
    }

    /* if there are queries waiting for a parent check first, handle
     * here
     */
    if (zlistx_size(waitcmds) > 0) {
        phased_power_on_check(cmd);
        send_initial_parent_queries(mh);
    }

    hostlist_iterator_destroy(itr);
    hostlist_destroy(lplugs);
}

static void on_cmd(CURLM *mh, char **av)
{
    power_cmd(mh, av, CMD_ON);
}

static void off_cmd(CURLM *mh, char **av)
{
    power_cmd(mh, av, CMD_OFF);
}

static void send_status_poll(struct powermsg *pm)
{
    struct powermsg *nextpm;
    char *path = NULL;
    long int poll_delay;

    get_path(CMD_STAT, pm->plugname, &path, NULL);
    if (!path) {
        printf("%s: %s path not set\n", pm->plugname, CMD_STAT);
        return;
    }

    /* testing a range of hardware shows that the amount of time it
     * takes to complete an on/off falls into two bands.  Either it
     * completes in the 2-8 second range OR it takes 20-60 seconds.
     *
     * Some example timings from a HPE Cray Supercomputing EX Chassis
     *
     * - Turn switch off - 1.18 seconds
     * - Turn switch on - 4.5 seconds
     * - Turn blade off - 1.18 seconds
     * - Turn blade on - 3.76 seconds
     * - Turn node off - 6.86 seconds
     * - Turn node on - 54.53 seconds
     *
     * (achu: Going off memory, the Supermicro H12DSG-O-CPU took
     * around 20 seconds for on/off.)
     *
     * To get the best turn around time for the quick end of that range
     * and avoid excessive polling on the other end, we will do a slightly
     * altered 'exponential backoff' delay.
     *
     * We delay 1 second each of the first 4 polls.
     * We delay 2 seconds for the 5th and 6th poll.
     * We delay 4 seconds afterwards.
     *
     * Special note, testing shows that powering on a "on" node can
     * also lead to a temporary entrance into the "PoweringOn" state.
     * So we also want a quick turnaround for that case, which is
     * typically only 1-2 seconds.
     */
    if (pm->poll_count < 4)
        poll_delay = status_polling_interval;
    else if (pm->poll_count < 6)
        poll_delay = status_polling_interval * 2;
    else
        poll_delay = status_polling_interval * 4;

    /* issue a follow on stat to wait until the on/off is complete.
     * note that we set the initial start time of this new command to
     * the original on/off, so we can timeout correctly
     *
     * in addition, the cmd is the original on/off, indicating the true
     * purpose of this status query
     */
    nextpm = powermsg_create(pm->mh,
                             pm->hostname,
                             pm->plugname,
                             pm->parent,
                             pm->cmd,
                             path,
                             NULL,
                             &pm->start,
                             poll_delay,
                             pm->poll_count + 1,
                             OUTPUT_RESULT,
                             STATE_WAIT_UNTIL_ON_OFF);
    if (!(nextpm->handle = zlistx_add_end(delayedcmds, nextpm)))
        err_exit(true, "zlistx_add_end");
    free(path);
}

static void on_off_process(struct powermsg *pm)
{
    if (pm->state == STATE_SEND_POWERCMD) {
        /* just sent on or off, now we need for the operation to
         * complete
         */
        send_status_poll(pm);

        /* in test mode, we simulate that the operation has already
         * finished */
        if (test_mode) {
            if (strcmp(pm->cmd, CMD_ON) == 0)
                zhashx_update(test_power_status, pm->plugname, STATUS_ON);
            else { /* cmd == CMD_OFF */
                zlistx_t *keys;
                char *name;
                zhashx_update(test_power_status, pm->plugname, STATUS_OFF);
                /* all children automatically become off too */
                if (!(keys = zhashx_keys(test_power_status)))
                    err_exit(false, "zhashx_keys");
                name = zlistx_first(keys);
                while (name) {
                    int descendant = plugs_is_descendant(plugs,
                                                         name,
                                                         pm->plugname);
                    if (descendant)
                        zhashx_update(test_power_status, name, STATUS_OFF);
                    name = zlistx_next(keys);
                }
                zlistx_destroy(&keys);
            }
        }
    }
    else if (pm->state == STATE_WAIT_UNTIL_ON_OFF) {
        const char *status_str;
        const char *rstatus_str;
        struct timeval now;

        parse_onoff(pm, &status_str, &rstatus_str);
        if (strcmp(status_str, pm->cmd) == 0) {
            printf("%s: %s\n", pm->plugname, "ok");
            process_waiters(pm->mh, pm->plugname, status_str);
            return;
        }

        /* check if we've timed out */
        gettimeofday(&now, NULL);
        if (timercmp(&now, &pm->timeout, >)) {
            /* if target is not what it should be, this is unexpected, likely
             * hardware problem */
            if ((strcmp(pm->cmd, CMD_ON) == 0
                 && strcmp(status_str, STATUS_OFF) == 0)
                || (strcmp(pm->cmd, CMD_OFF) == 0
                    && strcmp(status_str, STATUS_ON) == 0))
                printf("%s: timeout - unexpected %s\n",
                       pm->plugname, status_str);
            /* if still powering on/off, this is the "normal" timeout
             * scenario, timeout should be increased
             */
            else if ((strcmp(pm->cmd, CMD_ON) == 0
                      && strcmp(rstatus_str, STATUS_POWERING_ON) == 0)
                     || (strcmp(pm->cmd, CMD_OFF) == 0
                         && strcmp(rstatus_str, STATUS_POWERING_OFF) == 0))
                printf("%s: timeout - %s still in progress\n",
                       pm->plugname, pm->cmd);
            else {
                if (verbose)
                    printf("%s: timeout - unknown status %s\n",
                           pm->plugname, rstatus_str);
                else
                    printf("%s: timeout\n",
                           pm->plugname);
            }
            process_waiters(pm->mh, pm->plugname, STATUS_ERROR);
            return;
        }

        /* resend status poll */
        send_status_poll(pm);
    }

}

static void on_process(struct powermsg *pm)
{
    on_off_process(pm);
}

static void off_process(struct powermsg *pm)
{
    on_off_process(pm);
}

static void power_cmd_process(struct powermsg *pm)
{
    if (strcmp(pm->cmd, CMD_STAT) == 0)
        stat_process(pm);
    else if (strcmp(pm->cmd, CMD_ON) == 0)
        on_process(pm);
    else if (strcmp(pm->cmd, CMD_OFF) == 0)
        off_process(pm);
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
    if (userpwd_set_on_cmdline)
        return;
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

static void remove_initial_plugs(void)
{
    hostlist_iterator_t itr;
    char *hostname;

    if (!initial_plugs_setup)
        return;

    if (!(itr = hostlist_iterator_create(hosts)))
        err_exit(true, "hostlist_iterator_create");

    while ((hostname = hostlist_next(itr))) {
        plugs_remove(plugs, hostname);
        free(hostname);
    }

    hostlist_iterator_destroy(itr);
    initial_plugs_setup = 0;
    return;
}

static int setup_plug(const char *plugname,
                      const char *hostindexstr,
                      const char *parent)
{
    char *host;
    char *endptr;
    int hostindex;

    errno = 0;
    hostindex = strtol(hostindexstr, &endptr, 10);
    if (errno
        || endptr[0] != '\0'
        || hostindex < 0) {
        printf("setplugs: invalid hostindex %s specified\n", hostindexstr);
        return -1;
    }

    if (!(host = hostlist_nth(hosts, hostindex))) {
        printf("setplugs: hostindex %d out of range\n", hostindex);
        return -1;
    }

    plugs_add(plugs, plugname, host, parent);

    /* initialize plug to "off" for testing */
    if (test_mode)
        zhashx_insert(test_power_status, plugname, STATUS_OFF);

    free(host);
    return 0;
}

static void setplugs(char **av)
{
    hostlist_t lplugs = NULL;
    hostlist_t hostindices = NULL;
    int plugcount, hostindexcount;
    char *plug;
    char *hostindexstr;
    int i;

    if (!av[0] || !av[1]) {
        printf("Usage: setplugs <plugnames> <hostindices> [<parentplug>]]\n");
        return;
    }

    if (!(lplugs = hostlist_create(av[0]))) {
        printf("setplugs: illegal plugnames input\n");
        goto cleanup;
    }
    if (!(hostindices = hostlist_create(av[1]))) {
        printf("setplugs: illegal hostindices input\n");
        goto cleanup;
    }

    plugcount = hostlist_count(lplugs);
    hostindexcount = hostlist_count(hostindices);

    /* if user is electing to configure their own plugs, we must remove
     * all of the initial ones configured in setup_hosts()
     */
    remove_initial_plugs();

    if (plugcount != hostindexcount) {
        /* special case, user will use plug substitution */
        if (plugcount > 1 && hostindexcount == 1) {
            if (!(hostindexstr = hostlist_nth(hostindices, 0)))
                err_exit(false, "setplugs: hostlist_nth contains no indices");
            for (i = 0; i < plugcount; i++) {
                if (!(plug = hostlist_nth(lplugs, i)))
                    err_exit(false, "setplugs: hostlist_nth plugs");
                if (setup_plug(plug, hostindexstr, av[2]) < 0) {
                    free(plug);
                    free(hostindexstr);
                    goto cleanup;
                }
                free(plug);
            }
            free(hostindexstr);
        }
        else {
            printf("setplugs: plugs count not equal to host index count\n");
            goto cleanup;
        }
    }
    else {
        for (i = 0; i < plugcount; i++) {
            if (!(plug = hostlist_nth(lplugs, i)))
                err_exit(false, "setplugs: hostlist_nth plugs");
            if (!(hostindexstr = hostlist_nth(hostindices, i)))
                err_exit(false, "setplugs: hostlist_nth indices");
            if (setup_plug(plug, hostindexstr, av[2]) < 0) {
                free(plug);
                free(hostindexstr);
                goto cleanup;
            }
            free(plug);
            free(hostindexstr);
        }
    }

cleanup:
    hostlist_destroy(lplugs);
    hostlist_destroy(hostindices);
}

static void setpath(char **av)
{
    hostlist_t lplugs = NULL;
    hostlist_iterator_t itr = NULL;
    char *plugname;

    if (!av[0] || !av[1] || !av[2]) {
        printf("Usage: setpath <plugnames> <cmd> <path> [<postdata>]\n");
        return;
    }

    if (strcmp(av[1], CMD_STAT) != 0
        && strcmp(av[1], CMD_ON) != 0
        && strcmp(av[1], CMD_OFF) != 0) {
        printf("setpath: invalid command specified\n");
        return;
    }

    if (!(lplugs = hostlist_create(av[0]))) {
        printf("setpath: illegal hosts input\n");
        return;
    }
    itr = hostlist_iterator_create(lplugs);
    while ((plugname = hostlist_next(itr))) {
        if (!plugs_name_valid(plugs, plugname)) {
            printf("setpath: unknown plug specified: %s\n", plugname);
            free(plugname);
            goto cleanup;
        }
        if (plugs_update_path(plugs, plugname, av[1], av[2], av[3]) < 0)
            err_exit(false, "setpath: plugs_update_path failed");
        free(plugname);
    }
cleanup:
    hostlist_iterator_destroy(itr);
    hostlist_destroy(lplugs);
}

static void settimeout(char **av)
{
    if (av[0]) {
        char *endptr;
        long tmp;

        errno = 0;
        tmp = strtol(av[0], &endptr, 10);
        if (errno
            || endptr[0] != '\0'
            || tmp <= 0)
            printf("invalid timeout specified\n");
        cmd_timeout = tmp;
    }
}

static void process_cmd(CURLM *mh, char **av, int *exitflag)
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
        else if (strcmp(av[0], "setplugs") == 0)
            setplugs(av + 1);
        else if (strcmp(av[0], "setpath") == 0)
            setpath(av + 1);
        else if (strcmp(av[0], "settimeout") == 0)
            settimeout(av + 1);
        else if (strcmp(av[0], CMD_STAT) == 0)
            stat_cmd(mh, av + 1);
        else if (strcmp(av[0], CMD_ON) == 0)
            on_cmd(mh, av + 1);
        else if (strcmp(av[0], CMD_OFF) == 0)
            off_cmd(mh, av + 1);
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
    int exitflag = 0;

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

        if (!zlistx_size(activecmds)
            && !zlistx_size(delayedcmds)
            && !zlistx_size(waitcmds)) {
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
                process_cmd(mh, av, &exitflag);
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
                        if (cmsg->data.result == CURLE_HTTP_RETURNED_ERROR) {
                            /* N.B. curl returns this error code for all response
                             * codes >= 400.  So gotta dig in more.
                             */
                            long code;

                            if (curl_easy_getinfo(cmsg->easy_handle,
                                                  CURLINFO_RESPONSE_CODE,
                                                  &code) != CURLE_OK)
                                printf("%s: %s\n", pm->plugname, "http error");
                            if (code == 400)
                                printf("%s: %s\n", pm->plugname, "bad request");
                            else if (code == 401)
                                printf("%s: %s\n", pm->plugname, "unauthorized");
                            else if (code == 404)
                                printf("%s: %s\n", pm->plugname, "not found");
                            else
                                printf("%s: %s (%ld)\n",
                                       pm->plugname,
                                       "http error",
                                       code);
                        }
                        else
                            printf("%s: %s\n",
                                   pm->plugname,
                                   curl_easy_strerror(cmsg->data.result));
                        if (verbose)
                            printf("%s: %s\n", pm->plugname,
                                   curl_easy_strerror(cmsg->data.result));

                        process_waiters(mh,
                                        pm->plugname,
                                        STATUS_ERROR);
                    }
                    else
                        power_cmd_process(pm);
                    fflush(stdout);
                    if (zlistx_delete(activecmds, pm->handle) < 0)
                        err_exit(false, "zlistx_delete failed to delete");
                }
            } while (cmsg);
        }
        else {
            /* in test mode we assume all activecmds complete immediately */

            /* process_waiters() can traverse and append to the
             * activecmds list (it can also be called within
             * power_cmd_process()), thus disrupt the zlistx cursor.
             *
             * Copy all current activecmds into a new list and delete
             * them from activecmds at the end.
             */
            zlistx_t *cpy = zlistx_new();
            struct powermsg *pm;
            if (!cpy)
                err_exit(true, "zlistx_new");

            pm = zlistx_first(activecmds);
            while (pm) {
                /* do not save handle, we want handle for activecmds list */
                if (!zlistx_add_end(cpy, pm))
                    err_exit(true, "zlistx_add_end");
                pm = zlistx_next(activecmds);
            }

            pm = zlistx_first(cpy);
            while (pm) {
                if (hostlist_find(test_fail_power_cmd_hosts, pm->hostname) >= 0) {
                    printf("%s: %s\n", pm->plugname, "error");
                    process_waiters(mh,
                                    pm->plugname,
                                    STATUS_ERROR);
                }
                else
                    power_cmd_process(pm);
                fflush(stdout);
                pm = zlistx_next(cpy);
            }

            pm = zlistx_first(cpy);
            while (pm) {
                if (zlistx_delete(activecmds, pm->handle) < 0)
                    err_exit(false, "zlistx_delete failed to delete");
                pm = zlistx_next(cpy);
            }

            zlistx_destroy(&cpy);
        }
    }
}

static void usage(void)
{
    fprintf(stderr,
      "Usage: redfishpower --hostname host(s) [OPTIONS]\n"
      "  OPTIONS:\n"
      "  -H, --header          Set extra header string\n"
      "  -S, --statpath        Set stat path\n"
      "  -O, --onpath          Set on path\n"
      "  -F, --offpath         Set off path\n"
      "  -P, --onpostdata      Set on post data\n"
      "  -G, --offpostdata     Set off post data\n"
      "  -m, --message-timeout Set message timeout\n"
      "  -o, --resolve-hosts   Resolve host to IP before passing to libcurl\n"
      "  -v, --verbose         Increase output verbosity\n"
    );
    exit(1);
}

static void free_wrapper(void **item)
{
    if (item) {
        free(*item);
        *item = NULL;
    }
}

static void init_redfishpower(char *argv[])
{
    err_init(basename(argv[0]));

    if (!(hosts = hostlist_create(NULL)))
        err_exit(true, "hostlist_create error");

    if (!(activecmds = zlistx_new()))
        err_exit(true, "zlistx_new");
    zlistx_set_destructor(activecmds, cleanup_powermsg);

    if (!(delayedcmds = zlistx_new()))
        err_exit(true, "zlistx_new");
    zlistx_set_destructor(delayedcmds, cleanup_powermsg);

    if (!(waitcmds = zlistx_new()))
        err_exit(true, "zlistx_new");
    zlistx_set_destructor(waitcmds, cleanup_powermsg);

    if (!(test_fail_power_cmd_hosts = hostlist_create(NULL)))
        err_exit(true, "hostlist_create error");

    if (!(test_power_status = zhashx_new ()))
        err_exit(false, "zhashx_new error");

    if (!(plugs = plugs_create()))
        err_exit(true, "plugs_create");

    if (!(resolve_hosts_cache = zhashx_new ()))
        err_exit(false, "zhashx_new error");
    zhashx_set_destructor(resolve_hosts_cache, free_wrapper);
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

    zlistx_destroy(&activecmds);
    zlistx_destroy(&delayedcmds);
    zlistx_destroy(&waitcmds);

    hostlist_destroy(test_fail_power_cmd_hosts);
    zhashx_destroy(&test_power_status);

    plugs_destroy(plugs);

    zhashx_destroy(&resolve_hosts_cache);
}

static void setup_hosts(void)
{
    hostlist_iterator_t itr;
    char *hostname;

    if (!(itr = hostlist_iterator_create(hosts)))
        err_exit(true, "hostlist_iterator_create");

    /* initially all hosts on the command line are made the plugnames
     */
    while ((hostname = hostlist_next(itr))) {
        plugs_add(plugs, hostname, hostname, NULL);
        free(hostname);
    }

    hostlist_iterator_destroy(itr);

    initial_plugs_setup = 1;
}

/* From David Wheeler's Secure Programming Guide
 * - do not want compiler to optimize away memset()
 */
static void *secure_memset(void *s, int c, size_t n)
{
  volatile char *p;

  if (!s || !n)
    return (NULL);

  p = s;
  while (n--)
    *p++=c;

  return (s);
}

int main(int argc, char *argv[])
{
    CURLM *mh = NULL;
    CURLcode ec;
    int c;
    char *endptr;

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
            case 'A': /* -- auth */
                userpwd = xstrdup(optarg);
                userpwd_set_on_cmdline = 1;
                secure_memset(optarg, '\0', strlen(optarg));
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
            case 'm': /* --message-timeout */
                errno = 0;
                message_timeout = strtol(optarg, &endptr, 10);
                if (errno
                    || endptr[0] != '\0'
                    || message_timeout <= 0)
                    err_exit(false, "invalid message timeout specified\n");
                break;
            case 'o': /* --resolve_hosts */
                resolve_hosts = 1;
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

    setup_hosts();

    if (!test_mode) {
        if ((ec = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK)
            err_exit(false, "curl_global_init: %s", curl_easy_strerror(ec));

        if (!(mh = curl_multi_init()))
            err_exit(false, "curl_multi_init failed");
    }
    else {
        /* All hosts initially are off for testing */
        hostlist_t *lplugs = plugs_hostlist(plugs);
        hostlist_iterator_t itr;
        char *plugname;
        if (!(itr = hostlist_iterator_create(*lplugs)))
            err_exit(true, "hostlist_iterator_create");
        while ((plugname = hostlist_next(itr))) {
            if (zhashx_insert(test_power_status, plugname, STATUS_OFF) < 0)
                err_exit(false, "zhashx_insert failure");
            free(plugname);
        }
        hostlist_iterator_destroy(itr);

        /* under test mode we can make the polling interval a lot smaller
         * lets put it to a millisecond.
         */
        status_polling_interval = 1000;

        /* output settings of command line options that can't be tested in test mode */
        if (header)
            fprintf(stderr, "command line option: header = %s\n", header);
        if (userpwd)
            fprintf(stderr, "command line option: auth = %s\n", userpwd);
        if (message_timeout != MESSAGE_TIMEOUT_DEFAULT)
            fprintf(stderr, "command line option: message timeout = %ld\n", message_timeout);
        fprintf(stderr, "command line option: resolve-hosts = %s\n",
                resolve_hosts ? "set" : "not set");
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
