
typedef struct pm_handle_struct         *pm_handle_t;
typedef struct pm_node_iterator_struct  *pm_node_iterator_t;

typedef enum {
    PM_UNKNOWN      = 0,
    PM_OFF          = 1,
    PM_ON           = 2,
} pm_node_state_t;

typedef enum {
    PM_ESUCCESS     = 0,    /* success */
    PM_ERRNOVALID   = 1,    /* system call failed, see system errno */
    PM_ENOADDR      = 2,    /* failed to get address info for host:port */
    PM_ECONNECT     = 3,    /* connect failed */
    PM_ENOMEM       = 4,    /* out of memory */
    PM_EBADHAND     = 5,    /* bad server handle */
    PM_EBADITER     = 6,    /* bad node iterator */
    PM_EBADNODE     = 7,    /* unknown node name */
    PM_EBADARG      = 8,    /* bad argument */
    PM_ETIMEOUT     = 9,    /* client timed out */
    PM_ESERVEREOF   = 10,   /* received unexpected EOF from server */
    PM_SERVERERR    = 11,   /* server error */
} pm_err_t;

pm_err_t pm_connect(char *host, char *port, pm_handle_t *pmhp);
void     pm_disconnect(pm_handle_t pmh);

pm_err_t pm_node_iterator_create(pm_handle_t pmh, pm_node_iterator_t *pmip);
char *   pm_node_next(pm_node_iterator_t pmi);
void     pm_node_iterator_reset(pm_node_iterator_t pmi);
void     pm_node_iterator_destroy(pm_node_iterator_t pmi);

pm_err_t pm_node_status(pm_handle_t pmh, char *node, pm_node_state_t *statep);
pm_err_t pm_node_on(pm_handle_t pmh, char *node);
pm_err_t pm_node_off(pm_handle_t pmh, char *node);
pm_err_t pm_node_cycle(pm_handle_t pmh, char *node);

char *   pm_strerror(pm_err_t err, char *str, int len);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
