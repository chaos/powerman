/*****************************************************************************\
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Andrew Uselton <uselton2@llnl.gov>
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

#define _GNU_SOURCE             /* for dprintf */
#include <stdio.h>
#include <string.h>
#define _GNU_SOURCE
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>

#include "powerman.h"
#include "powerman_options.h"
#include "wrappers.h"
#include "error.h"
#include "hostlist.h"
#include "list.h"
#include "client_proto.h"
#include "debug.h"
#if !WITH_STATIC_MODULES
#include "ltdl.h"
#endif /* !WITH_STATIC_MODULES */

#if WITH_STATIC_MODULES
static void _load_one_module(struct powerman_options_module_info *module_info);
#else  /* !WITH_STATIC_MODULES */
static void _load_one_module(char *module_path);
static void _load_options_modules_in_dir(char *search_dir);
#endif /* !WITH_STATIC_MODULES */
static void _load_options_modules(void);
static void _unload_options_modules(void);
static void _connect_to_server(char *host, char *port);
static void _disconnect_from_server(void);
static void _usage(void);
static void _license(void);
static void _version(void);
static void _getline(char *str, int len);
static int _process_line(void);
static void _expect(char *str);
static int _process_response(void);
static void _process_version(void);

static int server_fd = -1;

#define POWERMAN_OPTIONS_LEN       128
#define POWERMAN_LONG_OPTIONS_LEN  64
#define POWERMAN_MAXPATHLEN        256

#define POWERMAN_MODULE_SIGNATURE  "powerman_options_"
#define POWERMAN_MODULE_INFO_SYM   "powerman_module_info"
#define POWERMAN_MODULE_DEVEL_DIR  POWERMAN_MODULE_BUILDDIR "/.libs"

struct powerman_module_loadinfo {
#if !WITH_STATIC_MODULES
  lt_dlhandle handle;
#endif /* !WITH_STATIC_MODULES */
  struct powerman_options_module_info *module_info;
  char options_towatch[POWERMAN_OPTIONS_LEN];
  char options_processed[POWERMAN_OPTIONS_LEN];
};

static List modules_list = NULL;
static ListIterator modules_list_itr = NULL;

#if WITH_STATIC_MODULES
extern struct powerman_options_module_info genders_module_info;

struct powerman_options_module_info *static_modules[] =
  {
#if WITH_GENDERS
    &genders_module_info,
#endif /* WITH_GENDERS */
    NULL
  };
#endif /* WITH_STATIC_MODULES */

#define OPT_STRING "01crlqfubntLd:VDvxT"
static char soptions[POWERMAN_OPTIONS_LEN + 1];

#if HAVE_GETOPT_LONG
static struct option loptions[POWERMAN_LONG_OPTIONS_LEN+1] = {
    {"on", no_argument, 0, '1'},
    {"off", no_argument, 0, '0'},
    {"cycle", no_argument, 0, 'c'},
    {"reset", no_argument, 0, 'r'},
    {"list", no_argument, 0, 'l'},
    {"query", no_argument, 0, 'q'},
    {"flash", no_argument, 0, 'f'},
    {"unflash", no_argument, 0, 'u'},
    {"beacon", no_argument, 0, 'b'},
    {"node", no_argument, 0, 'n'},
    {"temp", no_argument, 0, 't'},
    {"license", no_argument, 0, 'L'},
    {"destination", required_argument, 0, 'd'},
    {"version", no_argument, 0, 'V'},
    {"device", no_argument, 0, 'D'},
    {"telemetry", no_argument, 0, 'T'},
    {"exprange", no_argument, 0, 'x'},
    {0, 0, 0, 0}
};
static int loptions_len = 17;
#endif /* HAVE_GETOPT_LONG */

int main(int argc, char **argv)
{
    int c;
    int longindex;
    hostlist_t targets = NULL;
    bool have_targets = FALSE;
    char targstr[CP_LINEMAX];
    int res = 0;
    char *prog;
    enum { CMD_NONE, CMD_ON, CMD_OFF, CMD_LIST, CMD_CYCLE, CMD_RESET,
        CMD_QUERY, CMD_FLASH, CMD_UNFLASH, CMD_BEACON, CMD_TEMP, CMD_NODE,
        CMD_DEVICE
    } cmd = CMD_NONE;
    char *port = NULL;
    char *host = NULL;
    bool telemetry = FALSE;
    bool exprange = FALSE;
    int used_option;

    prog = basename(argv[0]);
    err_init(prog);

    if (strcmp(prog, "on") == 0)
        cmd = CMD_ON;
    else if (strcmp(prog, "off") == 0)
        cmd = CMD_OFF;

    memset(soptions, '\0', POWERMAN_OPTIONS_LEN + 1);
    strncpy(soptions, OPT_STRING, POWERMAN_OPTIONS_LEN);

    _load_options_modules();

    /* Build up acceptable options to parse */
    if (modules_list_itr) {
        struct powerman_module_loadinfo *loadinfoPtr;

        list_iterator_reset(modules_list_itr);
        while ((loadinfoPtr = list_next(modules_list_itr))) {
            struct powerman_option *optionsPtr = loadinfoPtr->module_info->options;

            while (optionsPtr->option) {
                char opt = optionsPtr->option;

                /* run out of space?  Then that's all the user gets */
                if (strlen(loadinfoPtr->options_towatch) >= POWERMAN_OPTIONS_LEN)
                    break;

                if (!strchr(soptions, optionsPtr->option)) {

                    /* run out of space?  Then that's all the user gets 
                     * -1 to account for possible ':'
                     */
                    if (strlen(soptions) >= (POWERMAN_OPTIONS_LEN - 1))
                        break;

#if HAVE_GETOPT_LONG
                    if (loptions_len >= POWERMAN_LONG_OPTIONS_LEN)
                        break;
#endif /* HAVE_GETOPT_LONG */

                    strncat(loadinfoPtr->options_towatch, &opt, 1);
                    strncat(soptions, &opt, 1);
                    if (optionsPtr->option_arg) {
                        opt = ':';
                        strncat(soptions, &opt, 1);
                    }

#if HAVE_GETOPT_LONG
                    if (optionsPtr->option_long)
                      {
                        loptions[loptions_len].name = optionsPtr->option_long;
                        if (optionsPtr->option_arg)
                          loptions[loptions_len].has_arg = 1;
                        else
                          loptions[loptions_len].has_arg = 0;
                        loptions[loptions_len].flag = NULL;
                        loptions[loptions_len].val = optionsPtr->option;
                        
                        loptions_len++;
                        loptions[loptions_len].name = NULL;
                        loptions[loptions_len].has_arg = 0;
                        loptions[loptions_len].flag = NULL;
                        loptions[loptions_len].val = 0;
                      }
#endif /* HAVE_GETOPT_LONG */
                }

                optionsPtr++;
            }
        }
    }

    /*
     * Parse options.
     */
    opterr = 0;
    while ((c = getopt_long(argc, argv, soptions, loptions,
                        &longindex)) != -1) {
        switch (c) {
        case 'l':              /* --list */
            cmd = CMD_LIST;
            break;
        case '1':              /* --on */
            cmd = CMD_ON;
            break;
        case '0':              /* --off */
            cmd = CMD_OFF;
            break;
        case 'c':              /* --cycle */
            cmd = CMD_CYCLE;
            break;
        case 'r':              /* --reset */
            cmd = CMD_RESET;
            break;
        case 'q':              /* --query */
            cmd = CMD_QUERY;
            break;
        case 'f':              /* --flash */
            cmd = CMD_FLASH;
            break;
        case 'u':              /* --unflash */
            cmd = CMD_UNFLASH;
            break;
        case 'b':              /* --beacon */
            cmd = CMD_BEACON;
            break;
        case 'n':              /* --node */
            cmd = CMD_NODE;
            break;
        case 't':              /* --temp */
            cmd = CMD_TEMP;
            break;
        case 'D':              /* --device */
            cmd = CMD_DEVICE;
            break;
        case 'd':              /* --destination host[:port] */
            if ((port = strchr(optarg, ':')))
                *port++ = '\0';  
            host = optarg;
            break;
        case 'L':              /* --license */
            _license();
            /*NOTREACHED*/
            break;
        case 'V':              /* --version */
            _version();
            /*NOTREACHED*/
            break;
        case 'T':              /* --telemetry */
            telemetry = TRUE;
            break;
        case 'x':              /* --exprange */
            exprange = TRUE;
            break;
        default:
            used_option = 0;

            if (modules_list_itr) {
                struct powerman_module_loadinfo *loadinfoPtr;

                list_iterator_reset(modules_list_itr);
                while ((loadinfoPtr = list_next(modules_list_itr))) {
                    char temp;

                    if (strchr(loadinfoPtr->options_towatch, c)) {
                        if ((*loadinfoPtr->module_info->process_option)(c, optarg) < 0)
                            err_exit(FALSE, "process_option failure");
                    }

                    temp = c;
                    strncat(loadinfoPtr->options_processed, &temp, 1);
                    used_option++;
                    break;
                }
            }

            if (used_option)
                break;
            /* else fall through */

        case '?':
            _usage();
            /*NOTREACHED*/
            break;
        }
    }

    if (cmd == CMD_NONE)
        _usage();

    /* get nodenames if desired */
    if (modules_list_itr) {
        struct powerman_module_loadinfo *loadinfoPtr;
        int break_flag = 0;

        list_iterator_reset(modules_list_itr);

        while ((loadinfoPtr = list_next(modules_list_itr)) && !break_flag) {
            struct powerman_option *optionsPtr = loadinfoPtr->module_info->options;
            while (optionsPtr->option) {
              
                if (optionsPtr->option_type == POWERMAN_OPTION_TYPE_GET_NODES
                    && strchr(loadinfoPtr->options_processed, optionsPtr->option)) {
                    char buf[CP_LINEMAX];
                    
                    if (!(*loadinfoPtr->module_info->get_nodes)(buf, CP_LINEMAX)) {
                        if (!have_targets) {
                            if ((targets = hostlist_create(NULL)) == NULL)
                                err_exit(FALSE, "hostlist error");
                            have_targets = TRUE;
                        }
                      
                        if (!hostlist_push(targets, buf))
                            err_exit(FALSE, "hostlist_push");

                        break_flag++;
                        break;
                    }
                }

                optionsPtr++;
            }
        }

    }

    /* remaining arguments used to build a single hostlist argument */
    while (optind < argc) {
        if (!have_targets) {
            if ((targets = hostlist_create(NULL)) == NULL)
                err_exit(FALSE, "hostlist error");
            have_targets = TRUE;
        }
        if (hostlist_push(targets, argv[optind]) == 0)
            err_exit(FALSE, "hostlist error");
        optind++;
    }
    if (have_targets) {
        hostlist_uniq(targets);
        hostlist_sort(targets);
        if (hostlist_ranged_string(targets, sizeof(targstr), targstr) == -1)
            err_exit(FALSE, "hostlist error");
    }

    /* verify a few option requirements */
    switch (cmd) {
    case CMD_LIST:
        if (have_targets)
            err_exit(FALSE, "option does not accept targets");
        break;
    case CMD_ON:
    case CMD_OFF:
    case CMD_RESET:
    case CMD_CYCLE:
    case CMD_FLASH:
    case CMD_UNFLASH:
        if (!have_targets)
            err_exit(FALSE, "option requires targets");
        break;
    default:
        break;
    }

    _connect_to_server(host ? host : DFLT_HOSTNAME, port ? port : DFLT_PORT);

    if (telemetry) {
        dprintf(server_fd, CP_TELEMETRY CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        if (res != 0)
            goto done;
    }

    if (exprange) {
        dprintf(server_fd, CP_EXPRANGE CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        if (res != 0)
            goto done;
    }

    /*
     * Execute the commands.
     */
    switch (cmd) {
    case CMD_LIST:
        dprintf(server_fd, CP_NODES CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_QUERY:
        if (have_targets)
            dprintf(server_fd, CP_STATUS CP_EOL, targstr);
        else
            dprintf(server_fd, CP_STATUS_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_ON:
        dprintf(server_fd, CP_ON CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_OFF:
        dprintf(server_fd, CP_OFF CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_RESET:
        dprintf(server_fd, CP_RESET CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_CYCLE:
        dprintf(server_fd, CP_CYCLE CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_FLASH:
        dprintf(server_fd, CP_BEACON_ON CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_UNFLASH:
        dprintf(server_fd, CP_BEACON_OFF CP_EOL, targstr);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_BEACON:
        if (have_targets)
            dprintf(server_fd, CP_BEACON CP_EOL, targstr);
        else
            dprintf(server_fd, CP_BEACON_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_TEMP:
        if (have_targets)
            dprintf(server_fd, CP_TEMP CP_EOL, targstr);
        else
            dprintf(server_fd, CP_TEMP_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_NODE:
        if (have_targets)
            dprintf(server_fd, CP_SOFT CP_EOL, targstr);
        else
            dprintf(server_fd, CP_SOFT_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_DEVICE:
        if (have_targets)
            dprintf(server_fd, CP_DEVICE CP_EOL, targstr);
        else
            dprintf(server_fd, CP_DEVICE_ALL CP_EOL);
        res = _process_response();
        _expect(CP_PROMPT);
        break;
    case CMD_NONE:
    default:
        _usage();
    }

done:
    _disconnect_from_server();
    _unload_options_modules();
    
    exit(res);
}

static void
#if WITH_STATIC_MODULES
_load_one_module(struct powerman_options_module_info *module_info)
#else  /* !WITH_STATIC_MODULES */
_load_one_module(char *module_path)
#endif /* !WITH_STATIC_MODULES */
{
    struct powerman_module_loadinfo *loadinfo;
    struct powerman_module_loadinfo *loadinfoPtr;
    struct powerman_option *optionsPtr;
    ListIterator itr = NULL;

#if WITH_STATIC_MODULES
    assert(module_info && modules_list);
#else  /* !WITH_STATIC_MODULES */
    assert(module_path && modules_list);
#endif /* !WITH_STATIC_MODULES */

    if (!(loadinfo = malloc(sizeof(struct powerman_module_loadinfo))))
        err_exit(TRUE, "malloc");
    memset(loadinfo, '\0', sizeof(struct powerman_module_loadinfo));

#if WITH_STATIC_MODULES
    loadinfo->module_info = module_info;
#else  /* !WITH_STATIC_MODULES */
    if (!(loadinfo->handle = lt_dlopen(module_path)))
        goto cleanup;

    if (!(loadinfo->module_info = lt_dlsym(loadinfo->handle, POWERMAN_MODULE_INFO_SYM)))
        goto cleanup;
#endif /* !WITH_STATIC_MODULES */

    if (!loadinfo->module_info->module_name
        || !loadinfo->module_info->options
        || !loadinfo->module_info->setup
        || !loadinfo->module_info->cleanup
        || !loadinfo->module_info->process_option
        || !loadinfo->module_info->get_nodes)
        goto cleanup;
    
    optionsPtr = loadinfo->module_info->options;
    while (optionsPtr->option) {
        /* Only one option type supported for now, maybe more in the future */
        if (optionsPtr->option_type != POWERMAN_OPTION_TYPE_GET_NODES)
            goto cleanup;
        optionsPtr++;
    }

    if (list_count(modules_list) > 0) {
        char *module_name;

        if (!(itr = list_iterator_create(modules_list)))
            err_exit(TRUE, "list_iterator_create");

        module_name = loadinfo->module_info->module_name;
        while ((loadinfoPtr = list_next(itr))) {
            if (!strcmp(loadinfoPtr->module_info->module_name, module_name))
                goto cleanup;           /* module already loaded */
        }
    }
    
    if ((*loadinfo->module_info->setup)() < 0)
        goto cleanup;

    if (!list_append(modules_list, loadinfo))
        err_exit(TRUE, "list_append");

    if (itr)
        list_iterator_destroy(itr);
    return;

 cleanup:
#if !WITH_STATIC_MODULES
    if (loadinfo) {
        if (loadinfo->handle)
            lt_dlclose(loadinfo->handle);
    }
#endif /* !WITH_STATIC_MODULES */
    if (itr)
        list_iterator_destroy(itr);
    return;
}

#if !WITH_STATIC_MODULES
static void 
_load_options_modules_in_dir(char *search_dir)
{
  DIR *dir;
  struct dirent *dirent;

  assert(search_dir);

  /* Can't open the directory? we assume it doesn't exit, so its not
   * an error.
   */
  if (!(dir = opendir(search_dir)))
    return;

  while ((dirent = readdir(dir))) {
      char *ptr = strstr(dirent->d_name, POWERMAN_MODULE_SIGNATURE);

      if (ptr && ptr == &dirent->d_name[0]) {
          char filebuf[POWERMAN_MAXPATHLEN+1];
          char *filename = dirent->d_name;

          /* Don't bother unless its a shared object file. */
          ptr = strchr(filename, '.');
          if (!ptr || strcmp(ptr, ".so"))
            continue;

          memset(filebuf, '\0', POWERMAN_MAXPATHLEN+1);
          snprintf(filebuf, POWERMAN_MAXPATHLEN, "%s/%s", search_dir, filename);

          _load_one_module(filebuf);
      }
  }
}
#endif /* !WITH_STATIC_MODULES */

static void
_delete_powerman_module_loadinfo(void *x)
{
    struct powerman_module_loadinfo *info = (struct powerman_module_loadinfo *)x;

    (*info->module_info->cleanup)();
#if !WITH_STATIC_MODULES
    lt_dlclose(info->handle);
#endif /* !WITH_STATIC_MODULES */
}

static void 
_load_options_modules(void)
{
#if WITH_STATIC_MODULES
  struct powerman_options_module_info **infoPtr;
#else  /* !WITH_STATIC_MODULES */
    if (lt_dlinit() != 0)
        err_exit(FALSE, "lt_dlinit: %s", lt_dlerror());
#endif /* !WITH_STATIC_MODULES */

    if (!(modules_list = list_create((ListDelF)_delete_powerman_module_loadinfo)))
        err_exit(TRUE, "list_create");

#if WITH_STATIC_MODULES
    infoPtr = &static_modules[0];
    while (*infoPtr) {
      _load_one_module(*infoPtr);
      infoPtr++;
    }
#else  /* !WITH_STATIC_MODULES */
#ifndef NDEBUG
    _load_options_modules_in_dir(POWERMAN_MODULE_DEVEL_DIR);
#endif /* !NDEBUG */
    _load_options_modules_in_dir(POWERMAN_MODULE_DIR);
#endif /* !WITH_STATIC_MODULES */

    if (list_count(modules_list) > 0) {
        if (!(modules_list_itr = list_iterator_create(modules_list)))
            err_exit(TRUE, "list_iterator_create");
    }
}

static void
_unload_options_modules(void)
{
    list_destroy(modules_list);
    modules_list = NULL;
#if !WITH_STATIC_MODULES
    lt_dlexit();
#endif /* !WITH_STATIC_MODULES */
}

/*
 * Display powerman usage and exit.
 */
static void _usage(void)
{
    printf("Usage: %s [OPTIONS] [TARGETS]\n", "powerman");
    printf(
 "-1 --on        Power on targets      -0 --off       Power off targets\n"
 "-q --query     Query plug status     -l --list      List available targets\n"
 "-c --cycle     Power cycle targets   -r --reset     Reset targets\n"
 "-f --flash     Turn beacon on        -u --unflash   Turn beacon off\n"
 "-b --beacon    Query beacon status   -n --node      Query node status\n"
 "-t --temp      Query temperature     -V --version   Report powerman version\n"
 "-D --device    Report device status  -T --telemetry Show device telemetry\n"
 "-x --exprange  Expand host ranges\n"
  );

    if (modules_list_itr) {
        struct powerman_module_loadinfo *loadinfoPtr;

        list_iterator_reset(modules_list_itr);
        while ((loadinfoPtr = list_next(modules_list_itr))) {
            struct powerman_option *optionsPtr = loadinfoPtr->module_info->options;
            while (optionsPtr->option) {
                if (optionsPtr->option_long) 
                    printf("-%c --%-9.9s %s\n", 
                           optionsPtr->option, 
                           optionsPtr->option_long,
                           optionsPtr->description);
                else
                    printf("-%c             %s\n", 
                           optionsPtr->option, 
                           optionsPtr->description);
                optionsPtr++;
            }
        }
    }
    exit(1);
}


/*
 * Display powerman license and exit.
 */
static void _license(void)
{
    printf(
 "Copyright (C) 2001-2002 The Regents of the University of California.\n"
 "Produced at Lawrence Livermore National Laboratory.\n"
 "Written by Andrew Uselton <uselton2@llnl.gov>.\n"
 "http://www.llnl.gov/linux/powerman/\n"
 "UCRL-CODE-2002-008\n\n"
 "PowerMan is free software; you can redistribute it and/or modify it\n"
 "under the terms of the GNU General Public License as published by\n"
 "the Free Software Foundation.\n");
    exit(1);
}

/*
 * Display powerman version and exit.
 */
static void _version(void)
{
    printf("%s\n", POWERMAN_VERSION);
    exit(1);
}

/*
 * Set up connection to server and get to the command prompt.
 */

static void _connect_to_server(char *host, char *port)
{
    struct addrinfo hints, *addrinfo;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    Getaddrinfo(host, port, &hints, &addrinfo);

    server_fd = Socket(addrinfo->ai_family, addrinfo->ai_socktype,
                       addrinfo->ai_protocol);

    if (Connect(server_fd, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0) {
        /* EINPROGRESS (not possible) | ECONNREFUSED */
        err_exit(TRUE, "powermand");
    }
    freeaddrinfo(addrinfo);

    _process_version();
    _expect(CP_PROMPT);
}

static void _disconnect_from_server(void)
{
    dprintf(server_fd, CP_QUIT CP_EOL);
    _expect(CP_RSP_QUIT);
}

/*
 * Return true if response should be suppressed.
 */
static bool _supress(int num)
{
    char *ignoreme[] = { CP_RSP_QRY_COMPLETE, CP_RSP_TELEMETRY, 
        CP_RSP_EXPRANGE };
    bool res = FALSE;
    int i;

    for (i = 0; i < sizeof(ignoreme)/sizeof(char *); i++) {
        if (strtol(ignoreme[i], (char **) NULL, 10) == num) {
            res = TRUE;
            break;
        }
    }

    return res;
}

/* 
 * Read a line of data terminated with \r\n or just \n.
 */
static void _getline(char *buf, int size)
{
    while (size > 1) {          /* leave room for terminating null */
        if (Read(server_fd, buf, 1) <= 0)
            err_exit(TRUE, "lost connection with server");
        if (*buf == '\r')
            continue;
        if (*buf == '\n')
            break;
        size--;
        buf++;
    }
    *buf = '\0';
}

/* 
 * Get a line from the socket and display on stdout.
 * Return the numerical portion of the repsonse.
 */
static int _process_line(void)
{
    char buf[CP_LINEMAX];
    long int num;

    _getline(buf, CP_LINEMAX);

    num = strtol(buf, (char **) NULL, 10);
    if (num == LONG_MIN || num == LONG_MAX)
        num = -1;
    if (strlen(buf) > 4) {
        if (!_supress(num))
            printf("%s\n", buf + 4);
    } else
        err_exit(FALSE, "unexpected response from server");
    return num;
}

/*
 * Read version and warn if it doesn't match the client's.
 */
static void _process_version(void)
{
    char buf[CP_LINEMAX], vers[CP_LINEMAX];

    _getline(buf, CP_LINEMAX);
    if (sscanf(buf, CP_VERSION, vers) != 1)
        err_exit(FALSE, "unexpected response from server");
    if (strcmp(vers, POWERMAN_VERSION) != 0)
        err(FALSE, "warning: server version (%s) != client (%s)",
                vers, POWERMAN_VERSION);
}

static int _process_response(void)
{
    int num;

    do {
        num = _process_line();
    } while (!CP_IS_ALLDONE(num));
    return (CP_IS_FAILURE(num) ? num : 0);
}

/* 
 * Read strlen(str) bytes from file descriptor and exit if
 * it doesn't match 'str'.
 */
static void _expect(char *str)
{
    char buf[CP_LINEMAX];
    int len = strlen(str);
    char *p = buf;
    int res;

    assert(len < sizeof(buf));
    do {
        res = Read(server_fd, p, len);
        if (res < 0)
            err_exit(TRUE, "lost connection with server");
        p += res;
        *p = '\0';
        len -= res;
    } while (strcmp(str, buf) != 0 && len > 0);

    /* Shouldn't happen.  We are not handling the general case of the server
     * returning the wrong response.  Read() loop above may hang in that case.
     */
    if (strcmp(str, buf) != 0)
        err_exit(FALSE, "unexpected response from server");
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
