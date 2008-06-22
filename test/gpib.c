/* gpib.c - gpib-utils hp3488/ics8064 simulator */

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

#include "hostlist.h"
#include "argv.h"
#include "xmalloc.h"

typedef enum { QUERY, OFF, ON } relayop_t;
typedef enum { NONE, ICS8064, HP3488 } gptype_t;

static void usage(void);
static void _noop_handler(int signum);
static void _zap_trailing_whitespace(char *s);
static void _prompt_loop(void);
static int relay_op(char *s, relayop_t op, int *plug, 
                    int num_plugs, int plug_origin);
static int ics8064_cmd(char **av, int plug[], 
                       int num_plugs, int plug_origin, int *qp);
static int hp3488_cmd(char **av, int plug[], 
                      int num_plugs, int plug_origin, int *qp);

static gptype_t personality = NONE;

static char *prog;

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
                if (strcmp(optarg, "hp3488") == 0)
                    personality = HP3488;
                else if (strcmp(optarg, "ics8064") == 0)
                    personality = ICS8064;
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
    fprintf(stderr, "Usage: %s -p ics8064|hp3488\n", prog);
    exit(1);
}

static void 
_noop_handler(int signum)
{
    fprintf(stderr, "%s: received signal %d\n", prog, signum);
}

static void 
_zap_trailing_whitespace(char *s)
{
    while (isspace(s[strlen(s) - 1]))
        s[strlen(s) - 1] = '\0';
}

static int
relay_op(char *s, relayop_t op, int *plug, int num_plugs, int plug_origin)
{
    /* N.B. WANT_RECKLESS_HOSTRANGE_EXPANSION is defined by gpib-utils,
     * but not by powerman.  Dev script will work but help cmd lies.
     */
    hostlist_t h = hostlist_create(s);
    hostlist_iterator_t it = hostlist_iterator_create(h);
    char *tmp;
    int i;
            
    while (tmp = hostlist_next(it)) {
        i = strtoul(tmp, NULL, 10);
        if (i < plug_origin || i >= num_plugs + plug_origin)
            break;
        if (op == ON) {
            plug[i - plug_origin] = 1;
        } else if (op == OFF) {
            plug[i - plug_origin] = 0;
        } else if (op == QUERY) {
            if (personality != HP3488)
                break;
            printf("%.3d: %d\n", i, plug[i - plug_origin]);
        }
    }
    hostlist_iterator_destroy(it);
    hostlist_destroy(h);

    return (tmp ? 0 : 1);
}

#define ICS8064_STATUS "ics8064>  %d,  %d,  %d,  %d,  %d,  %d,  %d,\
  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d,  %d\n"

static int
ics8064_cmd(char **av, int plug[], int num_plugs, int plug_origin, int *qp)
{
    if (argv_length(av) == 0) {
    } else if (!strcmp(av[0], "quit")) {
        *qp = 1;
    } else if (!strcmp(av[0], "help")) {
        printf("Commands:\n");
        printf("open relay-list       open the specified relay(s)\n");
        printf("close relay-list      close the specified relay(s)\n");
        printf("status                show relay status\n");
        printf("quit                  exit interactive mode\n");
        printf("Relay-list is comma-separated and may include ranges,\n");
        printf(" e.g.: \"1,2\", \"1-16\", or \"1-10,16\".\n");
    } else if (!strcmp(av[0], "status")) {
        if (argv_length(av) != 1)
            return 0;
        printf(ICS8064_STATUS, plug[0],  plug[1],  plug[2],  plug[3], 
                               plug[4],  plug[5],  plug[6],  plug[7], 
                               plug[8],  plug[9],  plug[10], plug[11], 
                               plug[12], plug[13], plug[14], plug[15]);
    } else if (!strcmp(av[0], "close")) { 
        if (argv_length(av) != 2)
            return 0;
        if (!relay_op(av[1], ON, plug, num_plugs, plug_origin))
            return 0;
    } else if (!strcmp(av[0], "open")) { 
        if (argv_length(av) != 2)
            return 0;
        if (!relay_op(av[1], OFF, plug, num_plugs, plug_origin))
            return 0;
    } else
        return 0;
    return 1;
}

static int
hp3488_cmd(char **av, int plug[], int num_plugs, int plug_origin, int *qp)
{
    if (argv_length(av) == 0) {
    } else if (!strcmp(av[0], "quit")) {
        *qp = 1;
    } else if (!strcmp(av[0], "help")) {
        printf("Possible commands are:\n");
        printf("   on [targets]    - turn on specified channels\n");
        printf("   off [targets]   - turn off specified channels\n");
        printf("   query [targets] - query specified channels (0=off,1=on)\n");
        printf("   list            - list valid caddrs\n");
        printf("   show            - show slot configuration\n");
        printf("   id              - query instrument id\n");
        printf("   verbose         - toggle verbose flag\n");
        printf("   help            - display this help text\n");
        printf("   quit            - exit this shell\n");
        printf("Where channel address is <slot><chan> (3 digits).\n");
    } else if (!strcmp(av[0], "query")) {
        if (argv_length(av) != 2)
            return 0;
        if (!relay_op(av[1], QUERY, plug, num_plugs, plug_origin))
            return 0;
    } else if (!strcmp(av[0], "on")) {
        if (argv_length(av) != 2)
            return 0;
        if (!relay_op(av[1], ON, plug, num_plugs, plug_origin))
            return 0;
    } else if (!strcmp(av[0], "off")) {
        if (argv_length(av) != 2)
            return 0;
        if (!relay_op(av[1], OFF, plug, num_plugs, plug_origin))
            return 0;
    } else
        return 0;
    return 1;
}

void
_prompt_loop(void)
{
    char buf[128];
    int num_plugs; 
    int plug_origin;
    int res, quit = 0;
    int *plug;
    char **av;

    if (personality == ICS8064) {
        num_plugs = 16;
        plug_origin = 1;
        plug = (int *)xmalloc(sizeof(int) * num_plugs);
    } else {
        num_plugs = 500; /* 100-599 */
        plug_origin = 100;
    }
 
    plug = (int *)xmalloc(sizeof(int) * num_plugs);
    memset(plug, 0, sizeof(int) * num_plugs);
    while (!quit) {
        if (personality == ICS8064)
            printf("ics8064> ");
        else
            printf("hp3488> ");
        if (fgets(buf, sizeof(buf), stdin)) {
            _zap_trailing_whitespace(buf);
            av = argv_create(buf, "");
            if (personality == ICS8064)
                res = ics8064_cmd(av, plug, num_plugs, plug_origin, &quit);
            else
                res = hp3488_cmd(av, plug, num_plugs, plug_origin, &quit);
            if (!res)
                printf("Unknown command\n");
            argv_destroy(av);
        } else
            break;
    }

    xfree(plug);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
