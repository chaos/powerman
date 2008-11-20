#include <stdio.h>
#include <stdlib.h>

#include "libpowerman.h"

int
main(int argc, char *argv[])
{
    pm_err_t err = PM_ESUCCESS;
    pm_node_state_t ns;
    pm_handle_t pm;
    char *str;

    if (argc != 3) {
        fprintf(stderr, "Usage: cli 0|1|q node\n");
        exit(1);
    }

    if ((err = pm_connect("localhost", "10103", &pm)) != PM_ESUCCESS) {
        fprintf(stderr, "pm_connect failed\n");
        exit(1);
    }

    if (!strcmp(argv[1], "1")) {
        err = pm_node_on(pm, argv[2]);
    } else if (!strcmp(argv[1], "0")) {
        err = pm_node_off(pm, argv[2]);
    } else if (!strcmp(argv[1], "q")) {
        if ((err = pm_node_status(pm, argv[2], &ns)) == PM_ESUCCESS)
            printf("%s\n", ns==PM_ON ? "on" : ns==PM_OFF ? "off" : "unknown");
    } else
        fprintf(stderr, "unknown command: %s\n", argv[1]);

    if (err != PM_ESUCCESS)
        fprintf(stderr, "an error occurred\n");

    pm_disconnect(pm);
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
