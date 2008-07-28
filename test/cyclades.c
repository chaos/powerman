/* cyclades.c - simulate cyclades power controllers */

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
#include <assert.h>

#include "xread.h"

static void usage(void);
static void _noop_handler(int signum);
static void _prompt_loop(void);

typedef enum { NONE, PM8, PM10 } cytype_t;

static char *prog;
static cytype_t personality = NONE;

#define OPTIONS "p:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    { "personality", required_argument, 0, 'p' },
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int 
main(int argc, char *argv[])
{
    int i, c;

    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
            case 'p':
                if (strcmp(optarg, "pm8") == 0)
                    personality = PM8;
                else if (strcmp(optarg, "pm10") == 0)
                    personality = PM10;
                else
                    usage();
                break;
            default:
                usage();
        }
    }
    if (optind < argc)
        usage();
    if (personality == NONE)
        usage();

    if (signal(SIGPIPE, _noop_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    _prompt_loop();
    exit(0);
}

static void 
usage(void)
{
    fprintf(stderr, "Usage: %s -p pm8|pm10\n", prog);
    exit(1);
}

static void 
_noop_handler(int signum)
{
    fprintf(stderr, "%s: received signal %d\n", prog, signum);
}


#define BANNER "\
AlterPath PM\n\
Copyright (c) 2002-2003 Cyclades Corporation\n\
V 1.0.9a Jun 26, 2003\n\n"

#define USER_PROMPT "Username: " /* admin */
#define PASS_PROMPT "Password: " /* pm8 */
#define CMD_PROMPT "pm>"

#define FAILED_MSG "\nAuthentication failed.\n"
                                                              
#define HELP_MSG "\n\n\n\
Available commands:\n\
\n\
adduser   alarm     assign   buzzer   current\n\
cycle     deluser   factory_defaults  help\n\
list      lock      name     off      on\n\
passwd    reboot    restore  save     status\n\
syslog    unassign  unlock   ver      whoami\n\
\n\
NOTE: To get detailed help on the commands listed above type\n\
      '<command> help';\n\
NOTE: Some commands accept as input a data type called \n\
      <outletString>. <outletString> is a string representing\n\
      one or more outlets. This string can be:\n\
      - one single outlet.\n\
        Examples: on 3 (turn on outlet 3);\n\
                  off router (turn off the outlet called router).\n\
      - a group of outlets.\n\
        Examples: status 1,3,5 (get status of outlets 1, 3 and 5);\n\
                  cycle 2-7 (cycle the outlets 2, 3, 4, 5, 6, 7)\n\
                  lock 2,5-7 (lock the outlets 2, 5, 6 and 7).\n"

#define STATUS_TMPL_PM8 "\
 Outlet         Name            Status          Users\n\
 1                              Unlocked %s\n\
 2                              Unlocked %s\n\
 3                              Unlocked %s\n\
 4                              Unlocked %s\n\
 5                              Unlocked %s\n\
 6                              Unlocked %s\n\
 7                              Unlocked %s\n\
 8                              Unlocked %s\n"

#define STATUS_TMPL_PM10 "\
 Outlet         Name            Status          Users\n\
 1                              Unlocked %s\n\
 2                              Unlocked %s\n\
 3                              Unlocked %s\n\
 4                              Unlocked %s\n\
 5                              Unlocked %s\n\
 6                              Unlocked %s\n\
 7                              Unlocked %s\n\
 8                              Unlocked %s\n\
 9                              Unlocked %s\n\
10                              Unlocked %s\n"

#define OFF_MSG "%d: Outlet turned off.\n"
#define ON_MSG  "%d: Outlet turned on.\n"

static void 
_prompt_loop(void)
{
    int i;
    char buf[128];
    int num_plugs;
    char plug[10][4];
    int plug_origin = 1;
    char user[32], pass[32];
    char status_all[32];
    char on_all[32];
    char off_all[32];
    char c;

    switch (personality) {
        case PM8:
            num_plugs = 8;
            break;
        case PM10:
            num_plugs = 10;
            break;
        case NONE:
            num_plugs = 0;
    }
    snprintf(status_all, sizeof(status_all), "status 1-%d", num_plugs);
    snprintf(on_all, sizeof(on_all), "on 1-%d", num_plugs);
    snprintf(off_all, sizeof(off_all), "off 1-%d", num_plugs);
    for (i = 0; i < num_plugs; i++)
        strcpy(plug[i], "OFF");


login_again:
    printf(BANNER);
    do {
        if (xreadline(USER_PROMPT, user, sizeof(user)) == NULL)
            goto done;
        if (xreadline(PASS_PROMPT, pass, sizeof(pass)) == NULL)
            goto done;
    } while (strcmp(user, "admin") != 0 || strcmp(pass, "pm8") != 0);

    while (1) {
        if (xreadline(CMD_PROMPT, buf, sizeof(buf)) == NULL) {
            goto done;
        } else if (strlen(buf) == 0) {
            continue;
        } else if (!strcmp(buf, "exit")) {
            sleep(1); 
            goto login_again;
        } else if (!strcmp(buf, "help")) {
            printf(HELP_MSG); 
        } else if (!strcmp(buf, status_all)) {
            if (personality == PM10) {
                printf(STATUS_TMPL_PM10, 
                       plug[0], plug[1], plug[2], plug[3],  
                       plug[4], plug[5], plug[6], plug[7],
                       plug[8], plug[9], plug[10]);
            } else if (personality == PM8) {
                printf(STATUS_TMPL_PM8, 
                       plug[0], plug[1], plug[2], plug[3],  
                       plug[4], plug[5], plug[6], plug[7]);
            }
        } else if (!strcmp(buf, on_all)) {
            for (i = 0; i < num_plugs; i++) {
                strcpy(plug[i], "ON");
                printf(ON_MSG, i);
            }
        } else if (sscanf(buf, "on %d", &i) == 1) {
            if (i >= plug_origin && i < num_plugs + plug_origin) {
                strcpy(plug[i - plug_origin], "ON");
                printf(ON_MSG, i);
            } else
                goto err;
        } else if (!strcmp(buf, off_all)) {
            for (i = 0; i < num_plugs; i++) {
                strcpy(plug[i], "OFF");
                printf(OFF_MSG, i);
            }
        } else if (sscanf(buf, "off %d", &i) == 1) {
            if (i >= plug_origin && i < num_plugs + plug_origin) {
                strcpy(plug[i - plug_origin], "OFF");
                printf(OFF_MSG, i);
            } else
                goto err;
        } else
            goto err;
        continue;
err:
        printf("Input error\r\n\r\n");
    }
done:
    return;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
