/*
 * libpowerman.c - simple powerman client library
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "powerman.h"
#include "client_proto.h"
#include "libpowerman.h"

#define PMH_MAGIC 0x44445555
struct pm_handle_struct {
    int         pmh_magic;
    int         pmh_fd;
    char **     pmh_nodenames;
    int         pmh_numnodes;
};

#define PMI_MAGIC 0xab5634fd
struct pm_node_iterator_struct {
    int         pmi_magic;
    pm_handle_t pmi_handle;
    int         pmi_pos;
};

typedef struct resp_t {
    char **lines;
    int count;
} resp_t;

/* Add a node to the list of nodes stored in the handle.
 */
static pm_err_t
_add_node(pm_handle_t pmh, char *node)
{
    char *nodecpy;

    if ((nodecpy = strdup(node)) == NULL)
        return PM_ENOMEM;
    if (pmh->pmh_nodenames == NULL)
        pmh->pmh_nodenames = malloc(sizeof(char *));
    else
        pmh->pmh_nodenames = realloc(pmh->pmh_nodenames, 
                                 (pmh->pmh_numnodes + 1)*sizeof(char *));
    if (pmh->pmh_nodenames == NULL) {
        free(nodecpy);
        return PM_ENOMEM;
    }
    pmh->pmh_nodenames[pmh->pmh_numnodes++] = nodecpy;
    return PM_ESUCCESS;
}

/* Establish connection to powerman server.
 */
static pm_err_t 
_connect_to_server_tcp(pm_handle_t pmh, char *host, char *port)
{
    struct addrinfo hints, *res, *r;
    pm_err_t result = PM_ESUCCESS;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0 || res == NULL) {
        result = PM_ENOADDR;
        goto done;
    }
    for (r = res; r != NULL; r = r->ai_next) {
        if ((pmh->pmh_fd = socket(r->ai_family, r->ai_socktype, 0)) < 0)
            continue;
        /* N.B. default TCP connect timeout applies */
        if (connect(pmh->pmh_fd, r->ai_addr, r->ai_addrlen) < 0) {
            close(pmh->pmh_fd);
            continue;
        }
        break; /* success! */
    }
    if (r == NULL)
        result = PM_ECONNECT;

done:
    if (res)
        freeaddrinfo(res);
    return result;
}

static int
_strncmpend(char *s1, char *s2, int len)
{
    return strncmp(s1 + len - strlen(s2), s2, strlen(s2));
}

static int
_terminated(char *buf, int count, pm_err_t *errp)
{
    /* success */
    if (_strncmpend(buf, CP_RSP_QUIT, count) == 0
                || _strncmpend(buf, CP_RSP_COM_COMPLETE, count) == 0
                || _strncmpend(buf, CP_RSP_QRY_COMPLETE, count) == 0
                || _strncmpend(buf, CP_RSP_TELEMETRY, count) == 0
                || _strncmpend(buf, CP_RSP_EXPRANGE, count) == 0) {
        *errp = PM_ESUCCESS;
        return 1;
    }

    /* failure */
    if (_strncmpend(buf, CP_ERR_UNKNOWN, count) == 0
                || _strncmpend(buf, CP_ERR_TOOLONG, count) == 0
                || _strncmpend(buf, CP_ERR_INTERNAL, count) == 0
                || _strncmpend(buf, CP_ERR_HOSTLIST, count) == 0
                || _strncmpend(buf, CP_ERR_CLIBUSY, count) == 0
                || _strncmpend(buf, CP_ERR_NOSUCHNODES, count) == 0
                || _strncmpend(buf, CP_ERR_COM_COMPLETE, count) == 0
                || _strncmpend(buf, CP_ERR_QRY_COMPLETE, count) == 0
                || _strncmpend(buf, CP_ERR_UNIMPL, count) == 0) {
        *errp = PM_ESERVER;
        return 1;
    }

    /* unterminated */    
    return 0;
}

static pm_err_t
_parse_response(char *buf, int len, resp_t *resp)
{
    int l = strlen(CP_EOL);
    int i, count = 0;  
    char *p, **lines = NULL;

    p = buf;
    for (i = 0; i < len - l; i++) {
        if (strncmp(&buf[i], CP_EOL, l) == 0)
            memset(&buf[i], 0, l);
        lines = lines ? (char **)realloc(lines, (count + 1) * sizeof(char *))
                      : (char **)malloc(sizeof(char *));
        if (lines == NULL)
            return PM_ENOMEM;
        lines[count++] = p;
        p = &buf[i + l];
    }
    if (resp) {
        resp->lines = lines;
        resp->count = count;
    }
    return PM_ESUCCESS;
}

static pm_err_t
_server_recv_response(pm_handle_t pmh, resp_t *resp)
{
    int buflen = 0, count = 0, n;
    char *buf = NULL; 
    pm_err_t err = PM_ESUCCESS;

    do {
        if (buflen - count == 0) {
            buflen += CP_LINEMAX;
            buf = buf ? realloc(buf, buflen) : malloc(buflen);
            if (buf == NULL) {
                err = PM_ENOMEM;
                break;
            }
        } 
        n = read(pmh->pmh_fd, buf + count, buflen - count);
        if (n == 0) {
            err = PM_ESERVEREOF;
            break;
        }
        if (n < 0) {
            err = PM_ERRNOVALID;
            break;
        }
        count += n;
    } while (!_terminated(buf, count, &err));

    if (err == PM_ESUCCESS && resp != NULL)
        err = _parse_response(buf, count, resp);
    if (err != PM_ESUCCESS && buf != NULL)
        free(buf);
    return err;
}

static pm_err_t
_server_send_command(pm_handle_t pmh, char *cmd, char *arg)
{
    char sendstr[CP_LINEMAX];    
    int count, msglen, n;
    pm_err_t err = PM_ESUCCESS;

    snprintf(sendstr, sizeof(sendstr), "%s%s%s%s", cmd, arg ? " " : "", 
                                                        arg ? arg : "", CP_EOL);
    count = 0;
    msglen = strlen(sendstr);
    while (count < msglen) {
        n = write(pmh->pmh_fd, sendstr + count, msglen - count);
        if (n < 0) {
            err = PM_ERRNOVALID;
            break;
        }
        count += n;
    }
    return err;
}

/* Send command to powerman server, with optional argument (if arg non-NULL),
 * and optionally collect response (if response non-NULL).
 * Caller must free response.
 */
static pm_err_t
_server_command(pm_handle_t pmh, char *cmd, char *arg, resp_t *resp)
{
    pm_err_t err;

    if ((err = _server_send_command(pmh, cmd, arg)) != PM_ESUCCESS)
        return err;
    if ((err = _server_recv_response(pmh, resp)) != PM_ESUCCESS)
        return err;
    return PM_ESUCCESS;
}


/* Connect to powerman server and return a handle for the connection.
 */
pm_err_t
pm_connect(char *host, char *port, pm_handle_t *pmhp)
{
    pm_handle_t pmh;
    pm_err_t err;
    resp_t resp;
    int i;

    if (pmhp == NULL)
        return PM_EBADARG;
    if (host == NULL)
        host = DFLT_HOSTNAME;
    if (port == NULL)
        port = DFLT_PORT;

    if ((pmh = (pm_handle_t)malloc(sizeof(struct pm_handle_struct))) == NULL)
        return PM_ENOMEM;

    pmh->pmh_magic = PMH_MAGIC;
    pmh->pmh_numnodes = 0;
    pmh->pmh_nodenames = NULL;

    if ((err = _connect_to_server_tcp(pmh, host, port)) != PM_ESUCCESS) {
        free(pmh);
        return err;
    }

    if ((err = _server_command(pmh, CP_EXPRANGE, NULL, NULL)) != PM_ESUCCESS)
        goto error;
    if ((err = _server_command(pmh, CP_NODES, NULL, &resp)) != PM_ESUCCESS)
        goto error;

    err = PM_ESUCCESS;
    for (i = 0; i < resp.count; i++) {
        char node[CP_LINEMAX];

        if (sscanf(resp.lines[i], "306 %s", node) == 1) {
            err = _add_node(pmh, node);
            if (err != PM_ESUCCESS)
                break;
        } else {
            err = PM_EPARSE;
            break;
        }
    }

    free(resp.lines);

    if (err == PM_ESUCCESS) {
        *pmhp = pmh;
        return PM_ESUCCESS;
    }

error:
    if (pmh != NULL) {
        (void)close(pmh->pmh_fd);
        for (i = 0; i < pmh->pmh_numnodes; i++)
            if (pmh->pmh_nodenames[i] != NULL)
                free(pmh->pmh_nodenames[i]);
        free(pmh->pmh_nodenames);
        free(pmh);
    }
    return err;
}


/* Disconnect from powerman server and free the handle.
 */
void 
pm_disconnect(pm_handle_t pmh)
{
    int i;

    if (pmh != NULL && pmh->pmh_magic == PMH_MAGIC) {
        (void)_server_command(pmh, "quit", NULL, NULL);
        (void)close(pmh->pmh_fd);

        if (pmh->pmh_nodenames != NULL) {
            for (i = 0; i < pmh->pmh_numnodes; i++)
                if (pmh->pmh_nodenames[i] != NULL)
                    free(pmh->pmh_nodenames[i]);
            free(pmh->pmh_nodenames);
        }
        free(pmh);
    }
}

pm_err_t
pm_node_iterator_create(pm_handle_t pmh, pm_node_iterator_t *pmip)
{
    pm_node_iterator_t pmi;

    if (pmh == NULL || pmh->pmh_magic != PMH_MAGIC)
        return PM_EBADHAND;
    if (pmip == NULL)
        return PM_EBADARG;

    pmi = (pm_node_iterator_t)malloc(sizeof(struct pm_node_iterator_struct));
    if (pmi == NULL)
        return PM_ENOMEM;
        
    pmi->pmi_magic = PMI_MAGIC;
    pmi->pmi_handle = pmh;
    pmi->pmi_pos = 0;

    *pmip = pmi;
    return PM_ESUCCESS;
}

void
pm_node_iterator_reset(pm_node_iterator_t pmi)
{
    if (pmi != NULL && pmi->pmi_magic == PMI_MAGIC)
        pmi->pmi_pos = 0;
}

char *
pm_node_next(pm_node_iterator_t pmi)
{
    if (pmi == NULL || pmi->pmi_magic != PMI_MAGIC)
        return NULL;
    if (pmi->pmi_pos >= pmi->pmi_handle->pmh_numnodes)
        return NULL;

    return pmi->pmi_handle->pmh_nodenames[pmi->pmi_pos++];
}

void
pm_node_iterator_destroy(pm_node_iterator_t pmi)
{
    if (pmi != NULL && pmi->pmi_magic == PMI_MAGIC) {
        memset(pmi, 0, sizeof(struct pm_node_iterator_struct));
        free(pmi);
    }
}

static pm_err_t
_validate_node(pm_handle_t pmh, char *node)
{
    pm_node_iterator_t pmi;
    char *tnode;
    pm_err_t err;

    if ((err = pm_node_iterator_create(pmh, &pmi)) != PM_ESUCCESS)
        return err;
    err = PM_EBADNODE;
    while ((tnode = pm_node_next(pmi)) != NULL) {
        if (!strcmp(node, tnode)) {
            err = PM_ESUCCESS;
            break;
        }
    }
    pm_node_iterator_destroy(pmi);
    return err;
}

pm_err_t
pm_node_status(pm_handle_t pmh, char *node, pm_node_state_t *statep)
{
    pm_err_t err;
    resp_t resp;

    if (pmh == NULL || pmh->pmh_magic != PMH_MAGIC)
        return PM_EBADHAND;
    if ((err = _validate_node(pmh, node)) != PM_ESUCCESS)
        return err;
    if ((err = _server_command(pmh, CP_STATUS, node, &resp)) != PM_ESUCCESS)
        return err;

     /* FIXME: translate response into pm_node_state_t */

    free(resp.lines);
    return PM_ESUCCESS;
}

pm_err_t
pm_node_on(pm_handle_t pmh, char *node)
{
    pm_err_t err;

    if (pmh == NULL || pmh->pmh_magic != PMH_MAGIC)
        return PM_EBADHAND;
    if ((err = _validate_node(pmh, node)) != PM_ESUCCESS)
        return err;
    if ((err = _server_command(pmh, CP_ON, node, NULL)) != PM_ESUCCESS)
        return err;

    return PM_ESUCCESS;
}
pm_err_t
pm_node_off(pm_handle_t pmh, char *node)
{
    pm_err_t err;

    if (pmh == NULL || pmh->pmh_magic != PMH_MAGIC)
        return PM_EBADHAND;
    if ((err = _validate_node(pmh, node)) != PM_ESUCCESS)
        return err;
    if ((err = _server_command(pmh, CP_OFF, node, NULL)) != PM_ESUCCESS)
        return err;

    return PM_ESUCCESS;
}

pm_err_t
pm_node_cycle(pm_handle_t pmh, char *node)
{
    pm_err_t err;

    if (pmh == NULL || pmh->pmh_magic != PMH_MAGIC)
        return PM_EBADHAND;
    if ((err = _validate_node(pmh, node)) != PM_ESUCCESS)
        return err;
    if ((err = _server_command(pmh, CP_CYCLE, node, NULL)) != PM_ESUCCESS)
        return err;

    return PM_ESUCCESS;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
