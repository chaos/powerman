/* baytech.c - baytech simulator */

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
static void _prompt_loop_rpc3_de(void);
static void _prompt_loop_rpc28_nc(void);
static void _prompt_loop_rpc3_nc(void);
static void _prompt_loop_rpc3(void);

typedef enum { NONE, RPC3, RPC3_NC, RPC28_NC, RPC3_DE } baytype_t;

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
    baytype_t personality = NONE;

    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
            case 'p':
                if (strcmp(optarg, "rpc3") == 0)
                    personality = RPC3;
                else if (strcmp(optarg, "rpc3-nc") == 0)
                    personality = RPC3_NC;
                else if (strcmp(optarg, "rpc28-nc") == 0)
                    personality = RPC28_NC;
                else if (strcmp(optarg, "rpc3-de") == 0)
                    personality = RPC3_DE;
                else
                    usage();
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

    switch (personality) {
        case NONE:
            usage();
        case RPC3:
            _prompt_loop_rpc3();
            break;
        case RPC3_NC:
            _prompt_loop_rpc3_nc();
            break;
        case RPC28_NC:
            _prompt_loop_rpc28_nc();
            break;
        case RPC3_DE:
            _prompt_loop_rpc3_de();
            break;
    }
    exit(0);
}

static void 
usage(void)
{
    fprintf(stderr, "Usage: %s -p rpc3|rpc3-nc|rpc28-nc|rpc3-de\n", prog);
    exit(1);
}

static void 
_noop_handler(int signum)
{
    fprintf(stderr, "%s: received signal %d\n", prog, signum);
}

#define RPC28_NC_BANNER "\
RPC-28 Series\n\
(C) 2000 by BayTech\n\
F3.01\n\
\n\
Option(s) Installed:\n\
True RMS Current\n\
Internal Temperature\n\
True RMS Voltage\n\
\n"

#define RPC28_NC_PROMPT "RPC-28>"
#define RPC28_NC_PROMPT2 "RPC-28A>"

#define RPC28_NC_STATUS "\r\n\
                    Outlet  1-10         Outlet 11-21 \r\n\
   Average Power:     619 Watts     :        5 Watts \r\n\
True RMS Voltage:   117.9 Volts     :    118.8 Volts \r\n\
True RMS Current:     5.4 Amps      :      0.1 Amps\r\n\
Maximum Detected:     6.9 Amps      :      2.8 Amps\r\n\
 Circuit Breaker:       Good        :        Good  \r\n\
\r\n\
Internal Temperature:  30.0 C\r\n\
\r\n\
\r\n\
 1)...Outlet  1       : %s           2)...Outlet  2       : %s          \r\n\
 3)...Outlet  3       : %s           4)...Outlet  4       : %s          \r\n\
 5)...Outlet  5       : %s           6)...Outlet  6       : %s          \r\n\
 7)...Outlet  7       : %s           8)...Outlet  8       : %s          \r\n\
 9)...Outlet  9       : %s          10)...Outlet 10       : %s          \r\n\
11)...Outlet 11       : %s          12)...Outlet 12       : %s          \r\n\
13)...Outlet 13       : %s          14)...Outlet 14       : %s          \r\n\
15)...Outlet 15       : %s          16)...Outlet 16       : %s          \r\n\
17)...Outlet 17       : %s          18)...Outlet 18       : %s          \r\n\
19)...Outlet 19       : %s          20)...Outlet 20       : %s          \r\n\
\r\n\
Type \"Help\" for a list of commands\r\n\
\r\n"

#define RPC28_NC_HELP "\r\n\
On n <cr>     --Turn on an Outlet, n=0,1...20,all\r\n\
Off n <cr>    --Turn off an Outlet, n=0,1...20,all\r\n\
Reboot n <cr> --Reboot an Outlet, n=0,1...20,all\r\n\
Status <cr>   --RPC-28 Status\r\n\
Config <cr>   --Enter configuration mode\r\n\
Lock n <cr>   --Locks Outlet(s) state, n=0,1...20,all\r\n\
Unlock n <cr> --Unlock Outlet(s) state, n=0,1...20,all\r\n\
Current <cr>  --Display True RMS Current\r\n\
Clear <cr>    --Reset the maximum detected current\r\n\
Temp <cr>     --Read current temperature\r\n\
Voltage <cr>  --Display True RMS Voltage\r\n\
Logout <cr>   --Logoff\r\n\
Logoff <cr>   --Logoff\r\n\
Exit <cr>     --Logoff\r\n\
Password <cr> --Changes the current user password\r\n\
Whoami <cr>   --Displays the current user name\r\n\
Unitid <cr>   --Displays the unit ID\r\n\
Help <cr>     --This Command\r\n\
\r\n\
\r\n\
Type \"Help\" for a list of commands\r\n\
\r\n"

#define RPC28_NC_TEMP "\r\n\
Internal Temperature:  30.0 C\r\n\
\r\n\
Type \"Help\" for a list of commands\r\n\
\r\n"

#define RPC28_NC_VOLTAGE "\r\n\
                    Outlet  1-10         Outlet 11-21 \r\n\
True RMS Voltage:   118.0 Volts     :    118.7 Volts \r\n\
\r\n\
Type \"Help\" for a list of commands\r\n\
\r\n"

#define RPC28_NC_CURRENT "\r\n\
                    Outlet  1-10         Outlet 11-21 \r\n\
True RMS Current:     5.5 Amps      :      0.1 Amps\r\n\
Maximum Detected:     6.9 Amps      :      2.8 Amps\r\n\
\r\n\
Type \"Help\" for a list of commands\r\n\
\r\n"

static void 
_prompt_loop_rpc28_nc(void)
{
    int i;
    char buf[128];
    int num_plugs = 20;
    char plug[20][4];
    int plug_origin = 1;
    int logged_in = 1;
    int seq = 0;

    for (i = 0; i < num_plugs; i++)
        strcpy(plug[i], "Off");

    printf(RPC28_NC_BANNER);

    while (logged_in) {
        /* switch between two possible prompts - our scripts must handle both */
        if (xreadline((seq++ % 2) ? RPC28_NC_PROMPT : RPC28_NC_PROMPT2,
                      buf, sizeof(buf)) == NULL)
            break;
        if (strlen(buf) == 0) {
            continue;
        } else if (!strcmp(buf, "logoff") || !strcmp(buf, "logout") 
                                          || !strcmp(buf, "exit")) {
            break;
        } else if (!strcmp(buf, "help")) {
            printf(RPC28_NC_HELP); 
        } else if (!strcmp(buf, "temp"))
            printf(RPC28_NC_TEMP); 
        else if (!strcmp(buf, "voltage"))
            printf(RPC28_NC_VOLTAGE); 
        else if (!strcmp(buf, "current"))
            printf(RPC28_NC_CURRENT); 
        else if (!strcmp(buf, "status")) {
            printf(RPC28_NC_STATUS, 
                   plug[0], plug[1], plug[2], plug[3],  
                   plug[4], plug[5], plug[6], plug[7],
                   plug[8], plug[9], plug[10], plug[11],  
                   plug[12], plug[13], plug[14], plug[15],
                   plug[16], plug[17], plug[18], plug[19]);
/* NOTE: we only suport one plug at a time or all for on,off,reboot */
        } else if (sscanf(buf, "on %d", &i) == 1) {
            if (i >= plug_origin && i < num_plugs + plug_origin)
                strcpy(plug[i - plug_origin], "On ");
            else if (i == 0)
                for (i = 0; i < num_plugs; i++)
                    strcpy(plug[i], "On ");
            else
                goto err;
        } else if (sscanf(buf, "off %d", &i) == 1) {
            if (i >= plug_origin && i < num_plugs + plug_origin)
                strcpy(plug[i - plug_origin], "Off");
            else if (i == 0)
                for (i = 0; i < num_plugs; i++)
                    strcpy(plug[i], "Off");
            else
                goto err;
        } else if (sscanf(buf, "reboot %d", &i) == 1) {
            /* if off, leaves it off */
            if (i == 0 || (i >= plug_origin && i < num_plugs + plug_origin)) {
                printf("\r\nRebooting...  ");
                for (i = 9; i >= 0; i--) {
                    printf("%d", i);
                    fflush(stdout);
                    sleep(1);
                }
                printf("\r\n");   
            } else
                goto err;
        } else
            goto err;
        continue;
err:
        printf("Input error\r\n\r\n");
    }
}

#define RPC3_DE_BANNER "\r\n\r\n\
RPC-3/4(A)DE Series\r\n\
(C) 2004 BayTech\r\n\
F1.08\r\n\
\r\n\
Option(s) Installed:\r\n\
True RMS Voltage\r\n\
True RMS Current\r\n\
Internal Temperature\r\n\
\r\n"

#define RPC3_DE_PROMPT "RPC-4>"

#define RPC3_DE_STATUS "\r\n\
\r\n\
-------------------------------------------------------------------------------\r\n\
|    Outlet      | True RMS  | Peak RMS  |  True RMS   |   Average  |  Volt-  |\r\n\
|    Group       |  Current  |  Current  |   Voltage   |    Power   |  Amps   |\r\n\
-------------------------------------------------------------------------------\r\n\
|  Outlet  1-4   |  1.8 Amps |  2.3 Amps | 122.5 Volts |  216 Watts |  220 VA |\r\n\
|  Outlet  5-8   |  2.4 Amps |  5.0 Amps | 122.1 Volts |  269 Watts |  277 VA |\r\n\
-------------------------------------------------------------------------------\r\n\
\r\n\
Internal Temperature:  78.8 F\r\n\
\r\n\
Switch 1: Open 2: Open\r\n\
\r\n\
 1)...Outlet  1       : %s           \r\n\
 2)...Outlet  2       : %s           \r\n\
 3)...Outlet  3       : %s           \r\n\
 4)...Outlet  4       : %s           \r\n\
 5)...Outlet  5       : %s           \r\n\
 6)...Outlet  6       : %s           \r\n\
 7)...Outlet  7       : %s           \r\n\
 8)...Outlet  8       : %s           \r\n\
\r\n\
\r\n\
Type Help for a list of commands\r\n\
\r\n"

#define RPC3_DE_HELP "\r\n\r\n\
On n <cr>     --Turn on an Outlet, n=0,1...8,all\r\n\
Off n <cr>    --Turn off an Outlet, n=0,1...8,all\r\n\
Reboot n <cr> --Reboot an Outlet, n=0,1...8,all\r\n\
Status <cr>   --RPC-4 Status\r\n\
Config <cr>   --Enter configuration mode\r\n\
RC <cr>       --Powerup/watchdog reset counts\r\n\
Lock n <cr>   --Locks Outlet(s) state, n=0,1...8,all\r\n\
Unlock n <cr> --Unlock Outlet(s) state, n=0,1...8,all\r\n\
Current <cr>  --Display True RMS Current\r\n\
Voltage <cr>  --Display True RMS Voltage\r\n\
Power <cr>    --Display Average Power\r\n\
Clear <cr>    --Reset the maximum detected current\r\n\
Temp <cr>     --Read current temperature\r\n\
Logout <cr>   --Logoff\r\n\
Logoff <cr>   --Logoff\r\n\
Exit <cr>     --Logoff\r\n\
Password <cr> --Changes the current user password\r\n\
Whoami <cr>   --Displays the current user name\r\n\
Unitid <cr>   --Displays the unit ID\r\n\
\r\n\
\r\n\
Type Help for a list of commands\r\n\
\r\n"

#define RPC3_DE_TEMP "\r\n\r\n\
Internal Temperature:  77.9 F\r\n\
\r\n\
Type Help for a list of commands\r\n\
\r\n"

#define RPC3_DE_VOLTAGE "\r\n\r\n\
--------------------------------\r\n\
|    Outlet      |  True RMS   |\r\n\
|    Group       |   Voltage   |\r\n\
--------------------------------\r\n\
|  Outlet  1-4   | 122.4 Volts | \r\n\
|  Outlet  5-8   | 122.3 Volts | \r\n\
--------------------------------\r\n\
\r\n\
Type Help for a list of commands\r\n\
\r\n"

#define RPC3_DE_CURRENT "\r\n\r\n\
------------------------------------------\r\n\
|    Outlet      | True RMS  | Peak RMS  |\r\n\
|    Group       |  Current  |  Current  |\r\n\
------------------------------------------\r\n\
|  Outlet  1-4   |  1.8 Amps |  2.3 Amps |\r\n\
|  Outlet  5-8   |  2.3 Amps |  5.0 Amps |\r\n\
------------------------------------------\r\n\
\r\n\
\r\n\
\r\n\
Type Help for a list of commands\r\n\
\r\n"

#define RPC3_DE_POWER "\r\n\r\n\
------------------------------------------\r\n\
|    Outlet      | True RMS  | Peak RMS  |\r\n\
|    Group       |  Current  |  Current  |\r\n\
------------------------------------------\r\n\
|  Outlet  1-4   |  1.8 Amps |  2.3 Amps |\r\n\
|  Outlet  5-8   |  2.3 Amps |  5.0 Amps |\r\n\
------------------------------------------\r\n\
\r\n\
\r\n\
\r\n\
Type Help for a list of commands\r\n\
\r\n"


static void 
_prompt_loop_rpc3_de(void)
{
    int i;
    char buf[128];
    int num_plugs = 8;
    char plug[8][4];
    int plug_origin = 1;
    int logged_in = 1;

    for (i = 0; i < num_plugs; i++)
        strcpy(plug[i], "Off");

    printf(RPC3_DE_BANNER);

    while (logged_in) {
        if (xreadline(RPC3_DE_PROMPT, buf, sizeof(buf)) == NULL)
            break;
        if (strlen(buf) == 0) {
            continue;
        } else if (!strcmp(buf, "logoff") || !strcmp(buf, "logout") 
                                          || !strcmp(buf, "exit")) {
            break;
        } else if (!strcmp(buf, "help")) {
            printf(RPC3_DE_HELP); 
        } else if (!strcmp(buf, "temp"))
            printf(RPC3_DE_TEMP); 
        else if (!strcmp(buf, "voltage"))
            printf(RPC3_DE_VOLTAGE); 
        else if (!strcmp(buf, "current"))
            printf(RPC3_DE_CURRENT); 
        else if (!strcmp(buf, "power"))
            printf(RPC3_DE_POWER); 
        else if (!strcmp(buf, "status")) {
            printf(RPC3_DE_STATUS, 
                   plug[0], plug[1], plug[2], plug[3],  
                   plug[4], plug[5], plug[6], plug[7]);
/* NOTE: we only suport one plug at a time or all for on,off,reboot */
        } else if (sscanf(buf, "on %d", &i) == 1) {
            if (i >= plug_origin && i < num_plugs + plug_origin)
                strcpy(plug[i - plug_origin], "On ");
            else if (i == 0)
                for (i = 0; i < num_plugs; i++)
                    strcpy(plug[i], "On ");
            else
                goto err;
        } else if (sscanf(buf, "off %d", &i) == 1) {
            if (i >= plug_origin && i < num_plugs + plug_origin)
                strcpy(plug[i - plug_origin], "Off");
            else if (i == 0)
                for (i = 0; i < num_plugs; i++)
                    strcpy(plug[i], "Off");
            else
                goto err;
        } else if (sscanf(buf, "reboot %d", &i) == 1) {
            /* if off, leaves it off */
            if (i == 0 || (i >= plug_origin && i < num_plugs + plug_origin)) {
                printf("\r\nRebooting...  ");
                for (i = 9; i >= 0; i--) {
                    printf("%d", i);
                    fflush(stdout);
                    sleep(1);
                }
                printf("\r\n");   
            } else
                goto err;
        } else
            goto err;
        continue;
err:
        printf("Input error\r\n\r\n");
    }
}

#define RPC3_NC_BANNER "\r\n\
\r\n\
RPC3-NC Series\r\n\
(C) 2002 by BayTech\r\n\
F4.00\r\n\
\r\n\
Option(s) Installed:\r\n\
True RMS Current\r\n\
Internal Temperature\r\n\
True RMS Voltage\r\n\
\r\n"
#define RPC3_NC_PROMPT "RPC3-NC>"

#define RPC3_NC_STATUS "\r\n\
\r\n\
   Average Power:     338 Watts\r\n\
True RMS Voltage:   120.9 Volts\r\n\
True RMS Current:     2.9 Amps\r\n\
Maximum Detected:     4.3 Amps\r\n\
 Circuit Breaker:       Good\r\n\
\r\n\
Internal Temperature:  40.0 C\r\n\
\r\n\
\r\n\
 1)...Outlet  1       : %s          \r\n\
 2)...Outlet  2       : %s          \r\n\
 3)...Outlet  3       : %s          \r\n\
 4)...Outlet  4       : %s          \r\n\
 5)...Outlet  5       : %s          \r\n\
 6)...Outlet  6       : %s          \r\n\
 7)...Outlet  7       : %s          \r\n\
 8)...Outlet  8       : %s          \r\n\
\r\n\
Type \"Help\" for a list of commands\r\n\
\r\n"

#define RPC3_NC_HELP "\r\n\
On n <cr>     --Turn on an Outlet, n=0,1...8,all\r\n\
Off n <cr>    --Turn off an Outlet, n=0,1...8,all\r\n\
Reboot n <cr> --Reboot an Outlet, n=0,1...8,all\r\n\
Status <cr>   --RPC3-NC Status\r\n\
Config <cr>   --Enter configuration mode\r\n\
Lock n <cr>   --Locks Outlet(s) state, n=0,1...8,all\r\n\
Unlock n <cr> --Unlock Outlet(s) state, n=0,1...8,all\r\n\
Current <cr>  --Display True RMS Current\r\n\
Clear <cr>    --Reset the maximum detected current\r\n\
Temp <cr>     --Read current temperature\r\n\
Voltage <cr>  --Display True RMS Voltage\r\n\
Logout <cr>   --Logoff\r\n\
Logoff <cr>   --Logoff\r\n\
Exit <cr>     --Logoff\r\n\
Password <cr> --Changes the current user password\r\n\
Whoami <cr>   --Displays the current user name\r\n\
Unitid <cr>   --Displays the unit ID\r\n\
Help <cr>     --This Command\r\n\
\r\n\
\r\n\
Type \"Help\" for a list of commands\r\n\
\r\n"

#define RPC3_NC_TEMP "\r\n\
Internal Temperature:  38.5 C\r\n\
\r\n\
Type \"Help\" for a list of commands\r\n\
\r\n"

#define RPC3_NC_VOLTAGE "\r\n\r\n\
True RMS Voltage:   120.5 Volts \r\n\
\r\n\
Type \"Help\" for a list of commands\r\n\
\r\n"

#define RPC3_NC_CURRENT "\r\n\r\n\
True RMS Current:     2.9 Amps\r\n\
Maximum Detected:     4.3 Amps\r\n\
\r\n\
\r\n\
Type \"Help\" for a list of commands\r\n\
\r\n"

static void 
_prompt_loop_rpc3_nc(void)
{
    int i;
    char buf[128];
    int num_plugs = 8;
    char plug[8][4];
    int plug_origin = 1;
    int logged_in = 1;

    for (i = 0; i < num_plugs; i++)
        strcpy(plug[i], "Off");

    printf(RPC3_NC_BANNER);

    while (logged_in) {
        if (xreadline(RPC3_NC_PROMPT, buf, sizeof(buf)) == NULL)
            break;
        if (strlen(buf) == 0) {
            continue;
        } else if (!strcmp(buf, "logoff") || !strcmp(buf, "logout") 
                                          || !strcmp(buf, "exit")) {
            break;
        } else if (!strcmp(buf, "help")) {
            printf(RPC3_NC_HELP); 
        } else if (!strcmp(buf, "temp"))
            printf(RPC3_NC_TEMP); 
        else if (!strcmp(buf, "voltage"))
            printf(RPC3_NC_VOLTAGE); 
        else if (!strcmp(buf, "current"))
            printf(RPC3_NC_CURRENT); 
        else if (!strcmp(buf, "status")) {
            printf(RPC3_NC_STATUS, 
                   plug[0], plug[1], plug[2], plug[3],  
                   plug[4], plug[5], plug[6], plug[7]);
/* NOTE: we only suport one plug at a time or all for on,off,reboot */
        } else if (sscanf(buf, "on %d", &i) == 1) {
            if (i >= plug_origin && i < num_plugs + plug_origin)
                strcpy(plug[i - plug_origin], "On ");
            else if (i == 0)
                for (i = 0; i < num_plugs; i++)
                    strcpy(plug[i], "On ");
            else
                goto err;
        } else if (sscanf(buf, "off %d", &i) == 1) {
            if (i >= plug_origin && i < num_plugs + plug_origin)
                strcpy(plug[i - plug_origin], "Off");
            else if (i == 0)
                for (i = 0; i < num_plugs; i++)
                    strcpy(plug[i], "Off");
            else
                goto err;
        } else if (sscanf(buf, "reboot %d", &i) == 1) {
            /* if off, leaves it off */
            if (i == 0 || (i >= plug_origin && i < num_plugs + plug_origin)) {
                printf("\r\nRebooting...  ");
                for (i = 9; i >= 0; i--) {
                    printf("%d", i);
                    fflush(stdout);
                    sleep(1);
                }
                printf("\r\n");   
            } else
                goto err;
        } else
            goto err;
        continue;
err:
        printf("Input error\r\n\r\n");
    }
}

#define RPC3_PROMPT "  RPC-3>"

#define RPC3_WELCOME "\
\r\n\
\r\n\
\r\n\
        RPC-3 Telnet Host\r\n\
    Revision F 5.01, (C) 2001 \r\n\
    Bay Technical Associates\r\n\
    Unit ID: BT RPC3-20\r\n"

#define RPC3_LOGIN "\
\r\n\
    Enter password>"

#define RPC3_BANNER "\
  Option(s) installed:\r\n\
  True RMS Current\r\n\
  Internal Temperature\r\n\
\r\n\
\r\n"

#define RPC3_MENU "\r\n\
  RPC-3 Menu:\r\n\
\r\n\
    1)...Outlet Control\r\n\
    2)...Manage Users\r\n\
    3)...Configuration\r\n\
    4)...Unit Status\r\n\
    5)...Reset Unit\r\n\
    6)...Logout\r\n\
\r\n\
  Enter Selection>"

#define RPC3_OUTLET "\
  True RMS current:  1.7 Amps\r\n\
  Maximum Detected:  2.5 Amps\r\n\
\r\n\
  Internal Temperature: 32.0 C\r\n\
\r\n\
  Circuit Breaker: On \r\n\
\r\n\
  Selection   Outlet    Outlet   Power\r\n\
    Number     Name     Number   Status\r\n\
      1       Outlet 1    1       %s \r\n\
      2       Outlet 2    2       %s \r\n\
      3       Outlet 3    3       %s \r\n\
      4       Outlet 4    4       %s \r\n\
      5       Outlet 5    5       %s \r\n\
      6       Outlet 6    6       %s \r\n\
      7       Outlet 7    7       %s \r\n\
      8       Outlet 8    8       %s \r\n\
\r\n\
  Type \"Help\" for a list of commands\r\n\
\r\n"

#define RPC3_OUTLET_HELP "\
  RPC3 Command Summary (F 5.01).\r\n\
  \"n\" refers to Selection Number, as displayed in outlet status\r\n\
  LOGOUT     : terminate session\r\n\
  OFF n      : turn off outlet \"n\", do all for n = 0\r\n\
  ON n       : turn on outlet \"n\", do all for n = 0\r\n\
  REBOOT n   : cycle power off/on outlet \"n\", do all for n = 0\r\n\
  RC         : display outlet relay control info\r\n\
  STATUS     : display power status of outlets\r\n\
  HELP       : display this message\r\n\
  CLEAR      : Reset the maximum detected current\r\n\
  CURRENT    : Read the current\r\n\
  TEMP       : Read current temperature\r\n\
  MENU       : return to main menu\r\n\
\r\n\
  <Strike CR to continue.>"

static void 
_prompt_loop_rpc3(void)
{
    int i;
    char buf[128];
    int num_plugs = 8;
    char plug[8][4];
    int plug_origin = 1;
    enum { START, MENU, OUTLET, QUIT } state = START;
    char *prompt;

    for (i = 0; i < num_plugs; i++)
        strcpy(plug[i], "Off");

    printf(RPC3_WELCOME);

    while (state != QUIT) {
        switch (state) {
            case START:
                prompt = RPC3_LOGIN;
                break;
            case MENU:
                prompt = RPC3_MENU;
                break;
            case OUTLET:
                printf(RPC3_OUTLET, plug[0], plug[1], plug[2], plug[3],
                                    plug[4], plug[5], plug[6], plug[7]);
                prompt = RPC3_PROMPT;
                break;
        }
        if (xreadline(prompt, buf, sizeof(buf)) == NULL) {
            state = QUIT;
            continue;
        }
        switch (state) {
            case START:
                if (!strcmp(buf, "baytech")) {
                    printf(RPC3_BANNER);
                    state = MENU;
                } else
                    printf("    Invalid password.\r\n\r\n");
                break;
            case MENU:
                if (!strcmp(buf, "1"))
                    state = OUTLET;
                else if (!strcmp(buf, "6"))
                    state = QUIT;
                else
                    goto err;
                break;
            case OUTLET:
                if (!strcmp(buf, "?") || !strcmp(buf, "help")) {
                    if (xreadline(RPC3_OUTLET_HELP, buf, sizeof(buf)) == NULL)
                        state = QUIT;
                } else if (!strcmp(buf, "menu")) {
                    state = MENU;
                } else if (!strcmp(buf, "logout")) {
                    state = QUIT;
                } else if (!strcmp(buf, "status")) {
                } else if (sscanf(buf, "on %d", &i) == 1) {
                    if (i >= plug_origin && i < num_plugs + plug_origin)
                        strcpy(plug[i - plug_origin], "On ");
                    else if (i == 0)
                        for (i = 0; i < num_plugs; i++)
                            strcpy(plug[i], "On ");
                    else
                        goto err;
                } else if (sscanf(buf, "off %d", &i) == 1) {
                    if (i >= plug_origin && i < num_plugs + plug_origin)
                        strcpy(plug[i - plug_origin], "Off");
                    else if (i == 0)
                        for (i = 0; i < num_plugs; i++)
                            strcpy(plug[i], "Off");
                    else
                        goto err;
                } else
                    goto err;
                break;
        }
        continue;
err:
        printf("\r\n  Input error.\r\n\r\n");
    }
}
        
/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
