/*****************************************************************************\
 *  $Id:$
 *****************************************************************************
 *  Copyright (C) 2007 The Regents of the University of California.
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

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <libgen.h>
#if HAVE_CURL
#include <curl/curl.h>
#else
#error Right now we really need curl support!
#endif
#if HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#else
#error Right now we really need readline support!
#endif
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include "wrappers.h"
#include "error.h"
#include "argv.h"

static char *url = NULL;
static char *userpwd = NULL;
static char errbuf[CURL_ERROR_SIZE];

#define OPTIONS "u:"
static struct option longopts[] = {
        {"url", required_argument, 0, 'u' },
        {0,0,0,0},
};

void help(void)
{
    printf("Valid commands are:\n");
    printf("  auth user:passwd\n");
    printf("  seturl url\n");
    printf("  get [url]\n");
    printf("  post [url] key=val[&key=val]...\n");
}

char *
_make_url(char *str)
{
    char *myurl = NULL;

    if (str && url) {
        myurl = Malloc(strlen(url) + strlen(str) + 2);
        sprintf(myurl, "%s/%s", url, str);
    } else if (str && !url) {
        myurl = Strdup(str);
    } else if (!str && url) {
        myurl = Strdup(url);
    }
    return myurl;
}

void post(CURL *h, char **av)
{
    char *myurl = _make_url(av[0]);
    char *postdata = av[1] ? Strdup(av[1]) : NULL;

    if (postdata && myurl) {
        curl_easy_setopt(h, CURLOPT_URL, myurl);
        curl_easy_setopt(h, CURLOPT_POSTFIELDS, postdata);
        if (curl_easy_perform(h) != 0)
            printf("Error: %s\n", errbuf);
        curl_easy_setopt(h, CURLOPT_URL, "");
        curl_easy_setopt(h, CURLOPT_POSTFIELDS, "");
    } else
        printf("Nothing to post!\n");

    if (myurl)
        Free(myurl);
    if (postdata)
        Free(postdata);
}

void get(CURL *h, char **av)
{
    char *myurl = _make_url(av[0]);

    if (myurl) {
        curl_easy_setopt(h, CURLOPT_URL, myurl);
        if (curl_easy_perform(h) != 0)
            printf("Error: %s\n", errbuf);
        curl_easy_setopt(h, CURLOPT_URL, "");
    } else 
        printf("Nothing to get!\n");

    if (myurl)
        Free(myurl);
}

void seturl(CURL *h, char **av)
{
    if (av[0] == NULL) {
        printf("Usage: seturl http://...\n");
        return;
    }
    if (url)
        Free(url);
    url = Strdup(av[0]);
}

void auth(CURL *h, char **av)
{
    if (av[0] == NULL) {
        printf("Usage: auth user:passwd\n");
        return;
    }
    if (userpwd)
        Free(userpwd);
    userpwd = Strdup(av[0]);
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
        else if (strcmp(av[0], "get") == 0)
            get(h, av + 1);
        else if (strcmp(av[0], "post") == 0)
            post(h, av + 1);
        else 
            printf("type \"help\" for a list of commands\n");
    }
    return rc;
}

void shell(CURL *h)
{
    char *line;
    char **av;
    int rc = 0;

    /* disable readline file name completion */
    /*rl_bind_key ('\t', rl_insert); */
    while (rc == 0 && (line = readline("httppower> "))) {
        if (strlen(line) > 0) {
            add_history(line);
            av = argv_create(line, "");
            rc = docmd(h, av);
            argv_destroy(av);
        }
        free(line);
    }
}

void
usage(void)
{
    fprintf(stderr, "Usage: httppower [--url URL]\n");
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
                url = Strdup(optarg);
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc)
        usage();

    if (curl_global_init(CURL_GLOBAL_ALL) != 0)
        err_exit(FALSE, "curl_global_init failed");
    if ((h = curl_easy_init()) == NULL)
        err_exit(FALSE, "curl_easy_init failed");

    curl_easy_setopt(h, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(h, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(h, CURLOPT_FAILONERROR, 1);

    shell(h);

    curl_easy_cleanup(h);    	
    if (userpwd)
        Free(userpwd);
    if (url)
        Free(url);
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
