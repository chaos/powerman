/*
 * libpowerman.c - a simple C API for controlling outlets via powerman
 */

#define PMH_MAGIC 0x44445555
struct pm_handle_struct {
    int pmh_magic;
};

#define PMI_MAGIC 0xab5634fd
struct pm_node_iterator_struct {
    int pmi_magic;
    int pmi_handle;
};

/* Connect to powerman server (host:port string format)
 * and return a handle for the connection.
 */
pm_err_t
pm_connect(char *servername, pm_handle_t *pmhp)
{
    pm_handle_t pmh;

    if (pmhp == NULL)
        return PM_EBADARG;

    /* FIXME: use external malloc function */
    pmh = (pm_handle_t)malloc(sizeof(struct pm_handle_struct));
    if (pmh == NULL)
        return PM_ENOMEM;

    pmh->pmh_magic = PMH_MAGIC;

    /* FIXME: connect to server */

    *pmhp = pmh;
    return pmh;
}

/* Disconnect from powerman server and free the handle.
 */
void 
pm_disconnect(pm_handle_t pmh)
{
    if (pmh == NULL || pmh->pmh_magic != PMH_MAGIC)
        return PM_EBADHAND;

    /* FIXME: disconnect from server */

    memset(pmh, 0, sizeof(struct pm_handle_struct));
    free(pmh);
}

pm_err_t
pm_node_iterator_create(pm_handle_t pmh, pm_node_iterator_t *pmip)
{
    pm_node_iterator_t pmi;

    if (pmh == NULL || pmh->pmh_magic != PMH_MAGIC)
        return PM_EBADHAND;
    if (pmip == NULL)
        return PM_BADARG;

    pmi = (pm_node_iterator_t)malloc(sizeof(struct pm_node_iterator_struct));
    if (pmi == NULL)
        return PM_ENOMEM;
        
    pmi->pmi_magic = PMI_MAGIC;
    pmi->pmi_handle = pmh;

    *pmip = pmi;
    return PM_SUCCESS;
}

pm_err_t
pm_node_iterator_destroy(pm_node_iterator_t pmi)
{
    if (pmi == NULL || pmi->pmi_magic != PMI_MAGIC)
        return PM_EBADITER;

    memset(pmi, 0, sizeof(struct pm_node_iterator_struct));
    free(pmi);

    return PM_SUCCESS;
}

pm_err_t
pm_node_next(pm_node_iterator_t pmi, char **next)
{
    if (pmi == NULL || pmi->pmi_magic != PMI_MAGIC)
        return PM_EBADITER;

    /* FIXME: return next node */

    return PM_SUCCESS;
}

pm_err_t
pm_node_state_query(pm_handle_t pmh, char *node, pm_node_state_t *oldstatep)
{
    if (pmh == NULL || pmh->pmh_magic != PMH_MAGIC)
        return PM_EBADHAND;

    /* FIXME: query node */

    return PM_SUCCESS;
}

pm_err_t
pm_node_state_update(pm_handle_t pmh, char *node, pm_node_state_t newstate)
{
    if (pmh == NULL || pmh->pmh_magic != PMH_MAGIC)
        return PM_EBADHAND;

    /* FIXME: update node */

    return PM_SUCCESS;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
