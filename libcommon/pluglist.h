#ifndef PM_PLUGLIST_H
#define PM_PLUGLIST_H

/*
 * Pluglists are used to map node names to plug names within a given device
 * context.
 */

typedef struct {
    char *name;                 /* how the plug is known to the device */
    char *node;                 /* node name */
} Plug;

typedef struct pluglist_iterator *PlugListIterator;
typedef struct pluglist *PlugList;

typedef enum { EPL_SUCCESS, EPL_DUPNODE, EPL_UNKPLUG, EPL_DUPPLUG,
               EPL_NOPLUGS, EPL_NONODES } pl_err_t;

/* Create a PlugList.  plugnames may be NULL (indicating plug creation is
 * deferred until pluglist_map() time), or a hardwired list of plug names.
 * ('hardwired' means the list of plugnames is fixed at creation time).
 */
PlugList          pluglist_create(List plugnames);

/* Destroy a PlugList and its Plugs
 */
void              pluglist_destroy(PlugList pl);

/* Assign node names to plugs.  nodelist and pluglist are strings in compressed
 * hostlist format (the degenerate case of which is a single name).  pluglist 
 * can be NULL, indicating 
 * - if hardwired plugs, available plugs should be assigned to nodes in order
 * - else (not hardwired), assume plug names are the same as node names.
 */
pl_err_t          pluglist_map(PlugList pl, char *nodelist, char *pluglist);

/* Search PlugList for a Plug with matching plug name.  Return pointer to Plug 
 * on success (points to actual list entry), or NULL on search failure.
 * Only plugs with non-NULL node fields are returned.
 */
Plug *            pluglist_find(PlugList pl, char *name);

/* An iterator interface for PlugLists, similar to the iterators in list.h.
 */
PlugListIterator  pluglist_iterator_create(PlugList pl);
void              pluglist_iterator_destroy(PlugListIterator itr);
Plug *            pluglist_next(PlugListIterator itr);

#endif /* PM_PLUGLIST_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
