/* cli.c - simple client to demo the libpowerman api */

#include <stdio.h>
#include <stdlib.h>

#include "libpowerman.h"

static pm_err_t query_one(pm_handle_t pm, char *s);
static pm_err_t query_all(pm_handle_t pm);

int
main(int argc, char *argv[])
{
    pm_err_t err = PM_ESUCCESS;
    pm_node_state_t ns;
    pm_handle_t pm;
    char *str, ebuf[64];
    char *host, *port;
    char cmd;

    if (argc < 4 || argc > 5 || (argv[3][0] != 'q' && argv[3][0] != '1'
                              && argv[3][0] != '0' && argv[3][0] != 'c')) {
        fprintf(stderr, "Usage: cli host port 0|1|c|q [node]\n");
        exit(1);
    }
    host = argv[1];
    port = argv[2];
    cmd = argv[3][0];

    if ((err = pm_connect(host, port, &pm)) != PM_ESUCCESS) {
        fprintf(stderr, "%s:%s: %s\n", host, port,
                pm_strerror(err, ebuf, sizeof(ebuf)));
        exit(1);
    }

    switch (cmd) {
        case '1':
            err = pm_node_on(pm, argv[4]);
            break;
        case '0':
            err = pm_node_off(pm, argv[4]);
            break;
        case 'c':
            err = pm_node_cycle(pm, argv[4]);
            break;
        case 'q':
            if (argc == 5)
                err = query_one(pm, argv[4]);
            else
                err = query_all(pm);
            break;
    }

    if (err != PM_ESUCCESS) {
        fprintf(stderr, "Error: %s\n", pm_strerror(err, ebuf, sizeof(ebuf)));
        pm_disconnect(pm);
        exit(1);
    }

    pm_disconnect(pm);
    exit(0);
}

static pm_err_t
query_one(pm_handle_t pm, char *s)
{
    pm_err_t err;
    pm_node_state_t ns;

    err = pm_node_status(pm, s, &ns);
    if (err == PM_ESUCCESS)
        printf("%s: %s\n", s, ns == PM_ON ? "on" : 
                              ns == PM_OFF ? "off" : "unknown");
    return err;
}

static pm_err_t
query_all(pm_handle_t pm)
{
    pm_err_t err;
    pm_node_iterator_t itr;
    char *s;

    err = pm_node_iterator_create(pm, &itr);
    if (err != PM_ESUCCESS)
        return err;
    while ((s = pm_node_next(itr)))
        if ((err = query_one(pm, s)) != PM_ESUCCESS)
            break;
    pm_node_iterator_destroy(itr);
    return err;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
