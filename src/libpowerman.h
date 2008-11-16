/* FIXME: c++ environment */

typedef struct pm_handle_struct         *pm_handle_t;
typedef struct pm_node_interator_struct *pm_node_interator_t;

enum {
    PM_OFF,
    PM_ON,
    PM_UNKNOWN,
} pm_node_state_t;

enum {
    PM_SUCCESS = 0,
    PM_ERRNOVALID,
    PM_ENOMEM,
    PM_EBADHAND,
    PM_EBADITER,
    PM_EBADARG,
    PM_SERVERERR,
} pm_err_t;

pm_err_t pm_connect(char *servername, pm_handle_t *pmhp);
pm_err_t pm_disconnect(pm_handle_t pmh);

pm_err_t pm_node_iterator_create(pm_handle_t pmh, pm_node_iterator_t *pmip);
pm_err_t pm_node_iterator_destroy(pm_node_iterator_t i);
pm_err_t pm_node_next(pm_node_iterator_t i, char **nextp);

pm_err_t pm_node_state_query(pm_handle_t pmh, char *node, 
                             pm_node_state_t *oldstatep);
pm_err_t pm_node_state_change(pm_handle_t pmh, char *node, 
                             pm_node_state_t newstate);

char *   pm_strerror(pm_err_t err, char *str, int len);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
