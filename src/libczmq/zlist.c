/*  =========================================================================
    zlist - simple generic list container

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of CZMQ, the high-level C binding for 0MQ:
    http://czmq.zeromq.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    Provides a generic container implementing a fast singly-linked list. You
    can use this to construct multi-dimensional lists, and other structures
    together with other generic containers like zhash. This is a simple
    class. For demanding applications we recommend using zlistx.
@discuss
    To iterate through a list, use zlist_first to get the first item, then
    loop while not null, and do zlist_next at the end of each iteration.
@end
*/

#include "czmq.h"
#include "czmq_internal.h"

//  List node, used internally only

typedef struct _node_t {
    struct _node_t *next;
    void *item;
    zlist_free_fn *free_fn;
} node_t;


//  ---------------------------------------------------------------------
//  Structure of our class

struct _zlist_t {
    node_t *head;                 //  First item in list, if any
    node_t *tail;                 //  Last item in list, if any
    node_t *cursor;               //  Current cursors for iteration
    size_t size;                  //  Number of items in list
    bool autofree;                //  If true, free items in destructor
    zlist_compare_fn *compare_fn; //  Function to compare two list item for
                                  //  less than, equals or greater than
};


//  --------------------------------------------------------------------------
//  List constructor

zlist_t *
zlist_new (void)
{
    zlist_t *self = (zlist_t *) zmalloc (sizeof (zlist_t));
    assert (self);
    return self;
}


//  --------------------------------------------------------------------------
//  List destructor

void
zlist_destroy (zlist_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zlist_t *self = *self_p;
        zlist_purge (self);
        freen (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Return the item at the head of list. If the list is empty, returns NULL.
//  Leaves cursor pointing at the head item, or NULL if the list is empty.

void *
zlist_first (zlist_t *self)
{
    assert (self);
    self->cursor = self->head;
    if (self->cursor)
        return self->cursor->item;
    else
        return NULL;
}


//  --------------------------------------------------------------------------
//  Return the next item. If the list is empty, returns NULL. To move to
//  the start of the list call zlist_first (). Advances the cursor.

void *
zlist_next (zlist_t *self)
{
    assert (self);
    if (self->cursor)
        self->cursor = self->cursor->next;
    else
        self->cursor = self->head;
    if (self->cursor)
        return self->cursor->item;
    else
        return NULL;
}


//  --------------------------------------------------------------------------
//  Return the item at the tail of list. If the list is empty, returns NULL.
//  Leaves cursor pointing at the tail item, or NULL if the list is empty.

void *
zlist_last (zlist_t *self)
{
    assert (self);
    self->cursor = self->tail;
    if (self->cursor)
        return self->cursor->item;
    else
        return NULL;
}


//  --------------------------------------------------------------------------
//  Return the item at the head of list. If the list is empty, returns NULL.
//  Leaves cursor as-is.

void *
zlist_head (zlist_t *self)
{
    assert (self);
    return self->head? self->head->item: NULL;
}


//  --------------------------------------------------------------------------
//  Return the item at the tail of list. If the list is empty, returns NULL.
//  Leaves cursor as-is.

void *
zlist_tail (zlist_t *self)
{
    assert (self);
    return self->tail? self->tail->item: NULL;
}


//  --------------------------------------------------------------------------
//  Return current item in the list. If the list is empty, or the cursor
//  passed the end of the list, returns NULL. Does not change the cursor.

void *
zlist_item (zlist_t *self)
{
    assert (self);
    if (self->cursor)
        return self->cursor->item;
    else
        return NULL;
}


//  --------------------------------------------------------------------------
//  Append an item to the end of the list, return 0 if OK or -1 if this
//  failed for some reason.

int
zlist_append (zlist_t *self, void *item)
{
    if (!item)
        return -1;

    node_t *node = (node_t *) zmalloc (sizeof (node_t));
    assert (node);

    //  If necessary, take duplicate of (string) item
    if (self->autofree) {
        item = strdup ((char *) item);
        assert (item);
    }
    node->item = item;
    if (self->tail)
        self->tail->next = node;
    else
        self->head = node;

    self->tail = node;
    node->next = NULL;

    self->size++;
    self->cursor = NULL;
    return 0;
}


//  --------------------------------------------------------------------------
//  Push an item to the start of the list, return 0 if OK or -1 if this
//  failed for some reason.

int
zlist_push (zlist_t *self, void *item)
{
    if (!item)
        return -1;

    node_t *node = (node_t *) zmalloc (sizeof (node_t));
    assert (node);

    //  If necessary, take duplicate of (string) item
    if (self->autofree) {
        item = strdup ((char *) item);
        assert (item);
    }
    node->item = item;
    node->next = self->head;
    self->head = node;
    if (self->tail == NULL)
        self->tail = node;

    self->size++;
    self->cursor = NULL;
    return 0;
}


//  --------------------------------------------------------------------------
//  Remove item from the beginning of the list, returns NULL if none

void *
zlist_pop (zlist_t *self)
{
    node_t *node = self->head;
    void *item = NULL;
    if (node) {
        item = node->item;
        self->head = node->next;
        if (self->tail == node)
            self->tail = NULL;
        freen (node);
        self->size--;
    }
    self->cursor = NULL;
    return item;
}


//  --------------------------------------------------------------------------
//  Checks if an item already is present. Uses compare method to determine if
//  items are equal. If the compare method is NULL the check will only compare
//  pointers. Returns true if item is present else false.

bool
zlist_exists (zlist_t *self, void *item)
{
    assert (self);
    assert (item);
    node_t *node = self->head;

    while (node) {
        if (self->compare_fn) {
            if ((*self->compare_fn)(node->item, item) == 0)
                return true;
        }
        else
        if (node->item == item)
            return true;

        node = node->next;
    }
    return false;
}


//  --------------------------------------------------------------------------
//  Remove the item from the list, if present. Safe to call on items that
//  are not in the list.

void
zlist_remove (zlist_t *self, void *item)
{
    node_t *node, *prev = NULL;

    //  First off, we need to find the list node
    for (node = self->head; node != NULL; node = node->next) {
        if (self->compare_fn) {
            if ((*self->compare_fn)(node->item, item) == 0)
               break;
        }
        else
        if (node->item == item)
            break;

        prev = node;
    }
    if (node) {
        if (prev)
            prev->next = node->next;
        else
            self->head = node->next;

        if (node->next == NULL)
            self->tail = prev;
        if (self->cursor == node)
            self->cursor = prev;

        if (self->autofree)
            freen (node->item);
        else
        if (node->free_fn)
            (node->free_fn)(node->item);

        freen (node);
        self->size--;
    }
}


//  --------------------------------------------------------------------------
//  Make a copy of list. If the list has autofree set, the copied list will
//  duplicate all items, which must be strings. Otherwise, the list will hold
//  pointers back to the items in the original list. If list is null, returns
//  NULL.

zlist_t *
zlist_dup (zlist_t *self)
{
    if (!self)
        return NULL;

    zlist_t *copy = zlist_new ();
    assert (copy);

    if (self->autofree)
        zlist_autofree(copy);

    copy->compare_fn = self->compare_fn;

    node_t *node;
    for (node = self->head; node; node = node->next) {
        if (zlist_append (copy, node->item) == -1) {
            zlist_destroy (&copy);
            break;
        }
    }
    return copy;
}


//  --------------------------------------------------------------------------
//  Purge all items from list

void
zlist_purge (zlist_t *self)
{
    assert (self);
    node_t *node = self->head;
    while (node) {
        node_t *next = node->next;
        if (self->autofree)
            freen (node->item);
        else
        if (node->free_fn)
            (node->free_fn)(node->item);

        freen (node);
        node = next;
    }
    self->head = NULL;
    self->tail = NULL;
    self->cursor = NULL;
    self->size = 0;
}


//  --------------------------------------------------------------------------
//  Return the number of items in the list

size_t
zlist_size (zlist_t *self)
{
    return self->size;
}


//  --------------------------------------------------------------------------
//  Sort the list. If the compare function is null, sorts the list by
//  ascending key value using a straight ASCII comparison. If you specify
//  a compare function, this decides how items are sorted. The sort is not
//  stable, so may reorder items with the same keys. The algorithm used is
//  combsort, a compromise between performance and simplicity.

void
zlist_sort (zlist_t *self, zlist_compare_fn compare_fn)
{
    zlist_compare_fn *compare = compare_fn;
    if (!compare) {
        compare = self->compare_fn;
        if (!compare)
            compare = (zlist_compare_fn *) strcmp;
    }
    //  Uses a comb sort, which is simple and reasonably fast.
    //  See http://en.wikipedia.org/wiki/Comb_sort
    size_t gap = self->size;
    bool swapped = false;
    while (gap > 1 || swapped) {
        if (gap > 1)
            gap = (size_t) ((double) gap / 1.3);
        node_t *base = self->head;
        node_t *test = self->head;
        size_t jump = gap;
        while (jump--)
            test = test->next;

        swapped = false;
        while (base && test) {
            if ((*compare) (base->item, test->item) > 0) {
                //  It's trivial to swap items in a generic container
                void *item = base->item;
                base->item = test->item;
                test->item = item;
                swapped = true;
            }
            base = base->next;
            test = test->next;
        }
    }
}


//  --------------------------------------------------------------------------
//  Sets a compare function for this list. The function compares two items.
//  It returns an integer less than, equal to, or greater than zero if the
//  first item is found, respectively, to be less than, to match, or be
//  greater than the second item.
//  This function is used for sorting, removal and exists checking.

void
zlist_comparefn (zlist_t *self,  zlist_compare_fn fn)
{
    assert (self);
    self->compare_fn = fn;
}


//  --------------------------------------------------------------------------
//  Set a free function for the specified list item. When the item is
//  destroyed, the free function, if any, is called on that item.
//  Use this when list items are dynamically allocated, to ensure that
//  you don't have memory leaks. You can pass 'free' or NULL as a free_fn.
//  Returns the item, or NULL if there is no such item.

void *
zlist_freefn (zlist_t *self, void *item, zlist_free_fn fn, bool at_tail)
{
    node_t *node = self->head;
    if (at_tail)
        node = self->tail;

    while (node) {
        if (node->item == item) {
            node->free_fn = fn;
            return item;
        }
        node = node->next;
    }
    return NULL;
}

//  --------------------------------------------------------------------------
//  Set list for automatic item destruction; item values MUST be strings.
//  By default a list item refers to a value held elsewhere. When you set
//  this, each time you append or push a list item, zlist will take a copy
//  of the string value. Then, when you destroy the list, it will free all
//  item values automatically. If you use any other technique to allocate
//  list values, you must free them explicitly before destroying the list.
//  The usual technique is to pop list items and destroy them, until the
//  list is empty.

void
zlist_autofree (zlist_t *self)
{
    assert (self);
    self->autofree = true;
}


#ifdef CZMQ_BUILD_EXTRA
static void
s_zlist_free (void *data)
{
    zlist_t *self = (zlist_t *) data;
    zlist_destroy (&self);
}

//  --------------------------------------------------------------------------
//  Runs selftest of class

void
zlist_test (bool verbose)
{
    printf (" * zlist: ");

    //  @selftest
    zlist_t *list = zlist_new ();
    assert (list);
    assert (zlist_size (list) == 0);

    //  Three items we'll use as test data
    //  List items are void *, not particularly strings
    char *cheese = "boursin";
    char *bread = "baguette";
    char *wine = "bordeaux";

    zlist_append (list, cheese);
    assert (zlist_size (list) == 1);
    assert ( zlist_exists (list, cheese));
    assert (!zlist_exists (list, bread));
    assert (!zlist_exists (list, wine));
    zlist_append (list, bread);
    assert (zlist_size (list) == 2);
    assert ( zlist_exists (list, cheese));
    assert ( zlist_exists (list, bread));
    assert (!zlist_exists (list, wine));
    zlist_append (list, wine);
    assert (zlist_size (list) == 3);
    assert ( zlist_exists (list, cheese));
    assert ( zlist_exists (list, bread));
    assert ( zlist_exists (list, wine));

    assert (zlist_head (list) == cheese);
    assert (zlist_next (list) == cheese);

    assert (zlist_first (list) == cheese);
    assert (zlist_tail (list) == wine);
    assert (zlist_next (list) == bread);

    assert (zlist_first (list) == cheese);
    assert (zlist_next (list) == bread);
    assert (zlist_next (list) == wine);
    assert (zlist_next (list) == NULL);
    //  After we reach end of list, next wraps around
    assert (zlist_next (list) == cheese);
    assert (zlist_size (list) == 3);

    zlist_remove (list, wine);
    assert (zlist_size (list) == 2);

    assert (zlist_first (list) == cheese);
    zlist_remove (list, cheese);
    assert (zlist_size (list) == 1);
    assert (zlist_first (list) == bread);

    zlist_remove (list, bread);
    assert (zlist_size (list) == 0);

    zlist_append (list, cheese);
    zlist_append (list, bread);
    assert (zlist_last (list) == bread);
    zlist_remove (list, bread);
    assert (zlist_last (list) == cheese);
    zlist_remove (list, cheese);
    assert (zlist_last (list) == NULL);

    zlist_push (list, cheese);
    assert (zlist_size (list) == 1);
    assert (zlist_first (list) == cheese);

    zlist_push (list, bread);
    assert (zlist_size (list) == 2);
    assert (zlist_first (list) == bread);
    assert (zlist_item (list) == bread);

    zlist_append (list, wine);
    assert (zlist_size (list) == 3);
    assert (zlist_first (list) == bread);

    zlist_t *sub_list = zlist_dup (list);
    assert (sub_list);
    assert (zlist_size (sub_list) == 3);

    zlist_sort (list, NULL);
    char *item;
    item = (char *) zlist_pop (list);
    assert (item == bread);
    item = (char *) zlist_pop (list);
    assert (item == wine);
    item = (char *) zlist_pop (list);
    assert (item == cheese);
    assert (zlist_size (list) == 0);

    assert (zlist_size (sub_list) == 3);
    zlist_push (list, sub_list);
    zlist_t *sub_list_2 = zlist_dup (sub_list);
    zlist_append (list, sub_list_2);
    assert (zlist_freefn (list, sub_list, &s_zlist_free, false) == sub_list);
    assert (zlist_freefn (list, sub_list_2, &s_zlist_free, true) == sub_list_2);
    zlist_destroy (&list);

    //  Test autofree functionality
    list = zlist_new ();
    assert (list);
    zlist_autofree (list);
    //  Set equals function otherwise equals will not work as autofree copies strings
    zlist_comparefn (list, (zlist_compare_fn *) strcmp);
    zlist_push (list, bread);
    zlist_append (list, cheese);
    assert (zlist_size (list) == 2);
    zlist_append (list, wine);
    assert (zlist_exists (list, wine));
    zlist_remove (list, wine);
    assert (!zlist_exists (list, wine));
    assert (streq ((const char *) zlist_first (list), bread));
    item = (char *) zlist_pop (list);
    assert (streq (item, bread));
    freen (item);
    item = (char *) zlist_pop (list);
    assert (streq (item, cheese));
    freen (item);

    zlist_destroy (&list);
    assert (list == NULL);

#if defined (__WINDOWS__)
    zsys_shutdown();
#endif
    //  @end

    printf ("OK\n");
}
#endif // CZMQ_BUILD_EXTRA
