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
#include <errno.h>

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

static int      _strncmpend(char *s1, char *s2, int len);
static char *   _strndup(char *s, int len);
static void *   _realloc(void *ptr, size_t size);
static pm_err_t _add_node(pm_handle_t pmh, char *node);
static pm_err_t _validate_node(pm_handle_t pmh, char *node);
static pm_err_t _connect_to_server_tcp(pm_handle_t pmh, char *host, char *port);
static pm_err_t _parse_response(char *buf, int len, resp_t *resp);
static pm_err_t _server_recv_response(pm_handle_t pmh, resp_t *resp);
static pm_err_t _server_send_command(pm_handle_t pmh, char *cmd, char *arg);
static pm_err_t _server_command(pm_handle_t pmh, char *cmd, char *arg, 
                                resp_t *resp);

/* Test if [s2] is a terminating substring of [s1].
 */
static int
_strncmpend(char *s1, char *s2, int len)
{
    return strncmp(s1 + len - strlen(s2), s2, strlen(s2));
}

/* Duplicate string [s] of length [len].
 * Result will be NULL terminated.  [s] doesn't have to be.
 */
static char *
_strndup(char *s, int len)
{
    char *c;

    if ((c = (char *)malloc(len + 1))) {
        memcpy(c, s, len);
        c[len] = '\0';
    }
    return c;     
}

/* Realloc that handles NULL [ptr].
 */
static void *
_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return malloc(size);
    return realloc(ptr, size);
}

/* Add [node] to the list of valid nodes stored in handle [pmh].
 */
static pm_err_t
_add_node(pm_handle_t pmh, char *node)
{
    char *nodecpy;

    if ((nodecpy = strdup(node)) == NULL)
        return PM_ENOMEM;
    pmh->pmh_nodenames = _realloc(pmh->pmh_nodenames, 
                                 (pmh->pmh_numnodes + 1)*sizeof(char *));
    if (pmh->pmh_nodenames == NULL) {
        free(nodecpy);
        return PM_ENOMEM;
    }
    pmh->pmh_nodenames[pmh->pmh_numnodes++] = nodecpy;
    return PM_ESUCCESS;
}

/* Establish connection to powerman server on [host] and [port].
 * Connection state is returned in the handle.
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

/* Parse a server response stored in [buf] of length [len] into
 * an array of lines stored in [resp].  Caller must free [resp].
 */
static pm_err_t
_parse_response(char *buf, int len, resp_t *resp)
{
    int l = strlen(CP_EOL);
    int i, count = 0;  
    char *p, **lines = NULL;
    char *cpy;
    pm_err_t err;

    p = buf;
    for (i = 0; i < len - l; i++) {
        if (strncmp(&buf[i], CP_EOL, l) != 0)
            continue;
        if ((cpy = _strndup(p, &buf[i] - p + l)) == NULL) {
            err = PM_ENOMEM;
            goto error;
        }
        lines = (char **)_realloc(lines, (count + 1) * sizeof(char *));
        if (lines == NULL) {
            err = PM_ENOMEM;
            goto error;
        }
        lines[count++] = cpy;
        p = &buf[i + l];
    }
    if (resp) {
        resp->lines = lines;
        resp->count = count;
    }

    return PM_ESUCCESS;
error:
    if (lines) {
        for (i = 0; i < count; i++)
            if (lines[i])
                free(lines[i]);
        free(lines);
    }
    return err;
}

/* Read response from server handle [pmh] and store it in
 * [resp], an array of lines.  Caller must free [resp].
 */
static pm_err_t
_server_recv_response(pm_handle_t pmh, resp_t *resp)
{
    int buflen = 0, count = 0, n;
    char *buf = NULL; 
    pm_err_t err = PM_ESUCCESS;

    do {
        if (buflen - count == 0) {
            buflen += CP_LINEMAX;
            buf = _realloc(buf, buflen);
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
    } while (_strncmpend(buf, CP_PROMPT, count) != 0);

    if (err == PM_ESUCCESS && resp != NULL)
        err = _parse_response(buf, count, resp);
    if (err != PM_ESUCCESS && buf != NULL)
        free(buf);
    return err;
}

/* Send command [cmd] with argument [arg] to server handle [pmh].
 * [cmd] is treated as a printf format string with [arg] as the
 * first printf argument (can be NULL).
 */
static pm_err_t
_server_send_command(pm_handle_t pmh, char *cmd, char *arg)
{
    char buf[CP_LINEMAX];    
    int count, len, n;
    pm_err_t err = PM_ESUCCESS;

    snprintf(buf, sizeof(buf), cmd, arg);
    strcat(buf, CP_EOL); /* FIXME */
    count = 0;
    len = strlen(buf);
    while (count < len) {
        n = write(pmh->pmh_fd, buf + count, len - count);
        if (n < 0) {
            err = PM_ERRNOVALID;
            break;
        }
        count += n;
    }
    return err;
}

/* Send command [cmd] with argumetn [arg] to server handle [pmh].
 * If [resp] is non-NULL, return parsed array of response lines 
 * in [resp] which the caller must free.
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

/* Check if [node] is one of the valid node names stored in handle [pmh].
 */
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

/* Connect to powerman server on [host] and [port], either of which
 * may be NULL indicating a desire to use the defaults.
 * Return a handle for the connection in [pmhp].
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

    /* eat version + prompt */
    if ((err = _server_recv_response(pmh, NULL)) != PM_ESUCCESS)
        goto error;
    /* tell server not to compress output */
    if ((err = _server_command(pmh, CP_EXPRANGE, NULL, NULL)) != PM_ESUCCESS)
        goto error;
    /* cache list of valid node names */
    if ((err = _server_command(pmh, CP_NODES, NULL, &resp)) != PM_ESUCCESS)
        goto error;

    err = PM_ESUCCESS;
    for (i = 0; i < resp.count; i++) {
        char node[CP_LINEMAX];

        if (sscanf(resp.lines[i], CP_INFO_XNODES, node) == 1) {
            err = _add_node(pmh, node);
            if (err != PM_ESUCCESS)
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


/* Disconnect from server handle [pmh] and free the handle.
 */
void 
pm_disconnect(pm_handle_t pmh)
{
    int i;

    if (pmh != NULL && pmh->pmh_magic == PMH_MAGIC) {
        (void)_server_command(pmh, CP_QUIT, NULL, NULL);
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

/* Create an iterater [pmip] that works over the list of valid node names 
 * stored at connect time in server handle [pmh].
 */
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

/* Rewind iterator [pmi] back to the beginning of the node list.
 */
void
pm_node_iterator_reset(pm_node_iterator_t pmi)
{
    if (pmi != NULL && pmi->pmi_magic == PMI_MAGIC)
        pmi->pmi_pos = 0;
}

/* Return the next node in the node list pointed to by iterator [pmi]
 * or NULL if end of list.
 */
char *
pm_node_next(pm_node_iterator_t pmi)
{
    if (pmi == NULL || pmi->pmi_magic != PMI_MAGIC)
        return NULL;
    if (pmi->pmi_pos >= pmi->pmi_handle->pmh_numnodes)
        return NULL;

    return pmi->pmi_handle->pmh_nodenames[pmi->pmi_pos++];
}

/* Destroy iterator [pmi].
 */
void
pm_node_iterator_destroy(pm_node_iterator_t pmi)
{
    if (pmi != NULL && pmi->pmi_magic == PMI_MAGIC) {
        memset(pmi, 0, sizeof(struct pm_node_iterator_struct));
        free(pmi);
    }
}

/* Query server [pmh] for the power status of [node], and store it
 * in [statep].
 */
pm_err_t
pm_node_status(pm_handle_t pmh, char *node, pm_node_state_t *statep)
{
    char offstr[CP_LINEMAX], onstr[CP_LINEMAX];
    pm_err_t err;
    resp_t resp;
    pm_node_state_t state = PM_UNKNOWN;
    int i;

    if (pmh == NULL || pmh->pmh_magic != PMH_MAGIC)
        return PM_EBADHAND;
    if ((err = _validate_node(pmh, node)) != PM_ESUCCESS)
        return err;
    if ((err = _server_command(pmh, CP_STATUS, node, &resp)) != PM_ESUCCESS)
        return err;

    snprintf(offstr, sizeof(offstr), CP_INFO_XSTATUS, node, "off");
    snprintf(onstr,  sizeof(onstr),  CP_INFO_XSTATUS, node, "on");

    for (i = 0; i < resp.count; i++) {
        if (!strcmp(offstr, resp.lines[i]))
            state = PM_OFF;
        else if (!strcmp(onstr, resp.lines[i]))
            state = PM_ON;
    }

    free(resp.lines);
    if (statep)
        *statep = state;
    return PM_ESUCCESS;
}

/* Tell server [pmh] to turn [node] on.
 */
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
    /* FIXME : parse result for success/failure */

    return PM_ESUCCESS;
}

/* Tell server [pmh] to turn [node] off.
 */
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
    /* FIXME : parse result for success/failure */

    return PM_ESUCCESS;
}

/* Tell server [pmh] to power cycle [node].
 */
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
    /* FIXME : parse result for success/failure */

    return PM_ESUCCESS;
}

char *
pm_strerror(pm_err_t err, char *str, int len)
{
    switch (err) {
        case PM_ESUCCESS:
            strncpy(str, "success", len);
            break;
        case PM_ERRNOVALID:
            strncpy(str, strerror(errno), len);
            break;
        case PM_ENOADDR:
            strncpy(str, "failed to get address info for host:port", len);
            break;
        case PM_ECONNECT:
            strncpy(str, "connect failed", len);
            break;
        case PM_ENOMEM:
            strncpy(str, "out of memory", len);
            break;
        case PM_EBADHAND:
            strncpy(str, "bad server handle", len);
            break;
        case PM_EBADNODE:
            strncpy(str, "unknown node name", len);
            break;
        case PM_EBADARG:
            strncpy(str, "bad argument", len);
            break;
        case PM_ETIMEOUT:
            strncpy(str, "client timed out", len);
            break;
        case PM_ESERVEREOF:
            strncpy(str, "received unexpected EOF from server", len);
            break;
        case PM_ESERVER:
            strncpy(str, "server error", len);
            break;
        case PM_EPARSE:
            strncpy(str, "received unexpected response from server", len);
            break;
    }
    return str;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
