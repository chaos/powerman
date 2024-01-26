/************************************************************\
 * Copyright (C) 2007 The Regents of the University of California.
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
#if HAVE_CURL_CURL_H
#include <curl/curl.h>
#else
#error httppower needs curl support!
#endif
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include "xmalloc.h"
#include "error.h"
#include "argv.h"

static char *url = NULL;
static char *header = NULL;
static struct curl_slist *header_list = NULL;
static int cookies = 0;
static int verbose = 0;
static char *userpwd = NULL;
static char errbuf[CURL_ERROR_SIZE];

#define OPTIONS "u:H:cv"
static struct option longopts[] = {
        {"url", required_argument, 0, 'u' },
        {"header", required_argument, 0, 'H' },
        {"cookies", no_argument, 0, 'c' },
        {"verbose", no_argument, 0, 'v' },
        {0,0,0,0},
};

void help(void)
{
    printf("Valid commands are:\n");
    printf("  auth user:passwd\n");
    printf("  seturl url\n");
    printf("  setheader string\n");
    printf("  cookies <enable|disable>\n");
    printf("  get [url]\n");
    printf("  post [url] <string data>\n");
    printf("  put [url] <string data>\n");
}

char *
_make_url(char *str)
{
    char *myurl = NULL;

    if (str && url) {
        myurl = xmalloc(strlen(url) + strlen(str) + 2);
        sprintf(myurl, "%s/%s", url, str);
    } else if (str && !url) {
        myurl = xstrdup(str);
    } else if (!str && url) {
        myurl = xstrdup(url);
    }
    return myurl;
}

void post(CURL *h, char **av)
{
    char *myurl = NULL;
    char *postdata = NULL;
    char *url_ptr = NULL;

    if (av[0] && av[1]) {
        postdata = xstrdup(av[1]);
        myurl = _make_url(av[0]);
        url_ptr = myurl;
    }
    else if (av[0]) {
        postdata = xstrdup(av[0]);
        url_ptr = url;
    }

    if (postdata && url_ptr) {
        curl_easy_setopt(h, CURLOPT_POST, 1);
        curl_easy_setopt(h, CURLOPT_URL, url_ptr);
        curl_easy_setopt(h, CURLOPT_POSTFIELDS, postdata);
        curl_easy_setopt(h, CURLOPT_POSTFIELDSIZE, strlen (postdata));
        if (curl_easy_perform(h) != 0)
            printf("Error: %s\n", errbuf);
        curl_easy_setopt(h, CURLOPT_URL, "");
        curl_easy_setopt(h, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(h, CURLOPT_POSTFIELDSIZE, 0);
    } else
        printf("Nothing to post!\n");

    if (myurl)
        xfree(myurl);
    if (postdata)
        xfree(postdata);
}

struct put_cb_data {
  char *data;
  int offset;
};

size_t put_read_cb(char *buffer, size_t size, size_t nitems, void *userdata) {
    struct put_cb_data *pcd = userdata;

    memcpy (buffer, pcd->data + pcd->offset, size);
    pcd->offset += size;
    return size;
}

void put(CURL *h, char **av)
{
    char *myurl = NULL;
    char *putdata = NULL;
    struct put_cb_data pcd;
    char *url_ptr = NULL;

    if (av[0] && av[1]) {
        putdata = xstrdup(av[1]);
        myurl = _make_url(av[0]);
        url_ptr = myurl;
    }
    else if (av[0]) {
        putdata = xstrdup(av[0]);
        url_ptr = url;
    }

    if (putdata && url_ptr) {
        curl_easy_setopt(h, CURLOPT_UPLOAD, 1);
        curl_easy_setopt(h, CURLOPT_URL, url_ptr);
        curl_easy_setopt(h, CURLOPT_READFUNCTION, put_read_cb);
        pcd.data = putdata;
        pcd.offset = 0;
        curl_easy_setopt(h, CURLOPT_READDATA, &pcd);
        curl_easy_setopt(h, CURLOPT_INFILESIZE, strlen (putdata));
        if (curl_easy_perform(h) != 0)
            printf("Error: %s\n", errbuf);
        curl_easy_setopt(h, CURLOPT_URL, "");
        curl_easy_setopt(h, CURLOPT_UPLOAD, 0);
    } else
        printf("Nothing to put!\n");

    if (myurl)
        xfree(myurl);
    if (putdata)
        xfree(putdata);
}

void get(CURL *h, char **av)
{
    char *myurl = _make_url(av[0]);

    if (myurl) {
        curl_easy_setopt(h, CURLOPT_HTTPGET, 1);
        curl_easy_setopt(h, CURLOPT_URL, myurl);
        if (curl_easy_perform(h) != 0)
            printf("Error: %s\n", errbuf);
        curl_easy_setopt(h, CURLOPT_URL, "");
    } else
        printf("Nothing to get!\n");

    if (myurl)
        xfree(myurl);
}

void seturl(CURL *h, char **av)
{
    if (av[0] == NULL) {
        printf("Usage: seturl http://...\n");
        return;
    }
    if (url)
        xfree(url);
    url = xstrdup(av[0]);
}

void setheader(CURL *h, char **av)
{
    if (header) {
        xfree(header);
        curl_slist_free_all(header_list);
        header = NULL;
        header_list = NULL;
    }
    if (av[0]) {
        header = xstrdup(av[0]);
        header_list = curl_slist_append(header_list, header);
        curl_easy_setopt(h, CURLOPT_HTTPHEADER, header_list);
    }
    else {
	curl_easy_setopt(h, CURLOPT_HTTPHEADER, header_list);
    }
}

void cookies_enable(CURL *h, char **av)
{
    if (av[0] == NULL
	|| (strcasecmp (av[0], "enable")
	    && strcasecmp (av[0], "disable"))) {
        printf("Usage: cookies <enable|disable>\n");
        return;
    }
    if (!strcasecmp (av[0], "enable")) {
        /* enable cookie engine with empty string, no need to read from
           a real file */
        curl_easy_setopt(h, CURLOPT_COOKIEFILE, "");
    }
    else {
	curl_easy_setopt(h, CURLOPT_COOKIELIST, "ALL");
        curl_easy_setopt(h, CURLOPT_COOKIEFILE, NULL);
    }
}

void auth(CURL *h, char **av)
{
    if (av[0] == NULL) {
        printf("Usage: auth user:passwd\n");
        return;
    }
    if (userpwd)
        xfree(userpwd);
    userpwd = xstrdup(av[0]);
    curl_easy_setopt(h, CURLOPT_USERPWD, userpwd);
    curl_easy_setopt(h, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
}

int docmd(CURL *h, char **av)
{
    int rc = 0;

    if (av[0] != NULL) {
        if (strcmp(av[0], "help") == 0)
            help();
        else if (strcmp(av[0], "quit") == 0)
            rc = 1;
        else if (strcmp(av[0], "auth") == 0)
            auth(h, av + 1);
        else if (strcmp(av[0], "seturl") == 0)
            seturl(h, av + 1);
        else if (strcmp(av[0], "setheader") == 0)
            setheader(h, av + 1);
        else if (strcmp(av[0], "cookies") == 0)
            cookies_enable(h, av + 1);
        else if (strcmp(av[0], "get") == 0)
            get(h, av + 1);
        else if (strcmp(av[0], "post") == 0)
            post(h, av + 1);
        else if (strcmp(av[0], "put") == 0)
            put(h, av + 1);
        else
            printf("type \"help\" for a list of commands\n");
    }
    return rc;
}

void shell(CURL *h)
{
    char buf[128];
    char **av;
    int rc = 0;

    while (rc == 0) {
        printf("httppower> ");
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin)) {
            av = argv_create(buf, "");
            rc = docmd(h, av);
            argv_destroy(av);
        } else
            rc = 1;
    }
}

void
usage(void)
{
    fprintf(stderr, "Usage: httppower [--url URL] [--header string] [--cookies]\n");
    exit(1);
}

int
main(int argc, char *argv[])
{
    CURL *h;
    int c;

    err_init(basename(argv[0]));

    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'u': /* --url */
                url = xstrdup(optarg);
                break;
            case 'H': /* --header */
                header = xstrdup(optarg);
                break;
	    case 'c': /* --cookies */
	        cookies = 1;
	        break;
            case 'v': /* --verbose */
                verbose = 1;
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc)
        usage();

    if (curl_global_init(CURL_GLOBAL_ALL) != 0)
        err_exit(false, "curl_global_init failed");
    if ((h = curl_easy_init()) == NULL)
        err_exit(false, "curl_easy_init failed");

    curl_easy_setopt(h, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(h, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(h, CURLOPT_FAILONERROR, 1);

    /* for time being */
    curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(h, CURLOPT_SSL_VERIFYHOST, 0L);

    if (verbose)
        curl_easy_setopt(h, CURLOPT_VERBOSE, 1L);

    if (header) {
        header_list = curl_slist_append(header_list, header);
        curl_easy_setopt(h, CURLOPT_HTTPHEADER, header_list);
    }

    /* enable cookie engine with empty string, no need to read from
       a real file */
    if (cookies)
        curl_easy_setopt(h, CURLOPT_COOKIEFILE, "");

    shell(h);

    curl_easy_cleanup(h);
    if (userpwd)
        xfree(userpwd);
    if (url)
        xfree(url);
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
