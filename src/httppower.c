#include <stdio.h>
#include <libgen.h>
#include <curl/curl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <stdlib.h>

#include "wrappers.h"
#include "error.h"
#include "argv.h"

void help(void)
{
    printf("Help!\n");
}

int docmd(CURL *h, char **av)
{
    int rc = 0;

    if (av[0] != NULL) {
        if (strcmp(av[0], "help") == 0)
            help();
        else if (strcmp(av[0], "quit") == 0)
            rc = 1;
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

int
main(int argc, char *argv[])
{
    CURL *h;

    err_init(basename(argv[0]));

    if (curl_global_init(CURL_GLOBAL_ALL) != 0)
        err_exit(FALSE, "curl_global_init failed");
    if ((h = curl_easy_init()) == NULL)
        err_exit(FALSE, "curl_easy_init failed");

    shell(h);

    curl_easy_cleanup(h);    	
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
