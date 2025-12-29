//
// Created by nilfer on 12/29/25.
//

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"


#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1
#define DPLIST_INVALID_ERROR 2

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;
    void *(*element_copy)(void *src_element);
    void (*element_free)(void **element);
    int (*element_compare)(void *x, void *y);
};

dplist_t *dpl_create(
        void *(*element_copy)(void *element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    assert(list != NULL); // we use assert to check memory allocation

    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;

    return list;
}

void dpl_free(dplist_t **list, bool free_element) {
    if (list == NULL || *list == NULL) {
        return;
    }

    // We can reuse dpl_remove_at_index to clear the list
    // i keep removing index 0 repeatedly until the list is completely empty
    while ((*list)->head != NULL) {
        dpl_remove_at_index(*list, 0, free_element);
    }

    free(*list);
    *list = NULL;
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {
    if (list == NULL) return NULL;

    dplist_node_t *ref_at_index, *list_node;

    list_node = malloc(sizeof(dplist_node_t));
    assert(list_node != NULL);


    if (insert_copy && list->element_copy) {
        list_node->element = list->element_copy(element);
    } else {
        list_node->element = element;
    }

    // sit1: if the list is empty
    if (list->head == NULL) {
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
        return list;
    }

    // sit2: insert at the start (index <= 0)
    if (index <= 0) {
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
        return list;
    }

    // use this to find the correct position
    ref_at_index = list->head;
    int i = 0;
    while (ref_at_index->next != NULL && i < index - 1) {
        ref_at_index = ref_at_index->next;
        i++;
    }

    // sit3: Insert at the end or specific index
    // here we choose 'ref_at_index' as specific index and insert after that
    list_node->next = ref_at_index->next;
    list_node->prev = ref_at_index;

    if (ref_at_index->next != NULL) {
        ref_at_index->next->prev = list_node;
    }
    ref_at_index->next = list_node;

    return list;
}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {
    if (list == NULL || list->head == NULL) return list;

    dplist_node_t *node_to_remove;

    // situation1: Remove from start (index <= 0)
    if (index <= 0) {
        node_to_remove = list->head;
        list->head = list->head->next;
        if (list->head) {
            list->head->prev = NULL;
        }
    } else {
        // sit2: Remove from specific index or end
        dplist_node_t *ref_at_index = list->head;
        int i = 0;
        // Stop at the node to remove
        while (ref_at_index->next != NULL && i < index) {
            ref_at_index = ref_at_index->next;
            i++;
        }
        node_to_remove = ref_at_index;


        if (node_to_remove->prev) {
            node_to_remove->prev->next = node_to_remove->next;
        }
        if (node_to_remove->next) {
            node_to_remove->next->prev = node_to_remove->prev;
        }
    }

    // Free the element data but only if requested!
    if (free_element && list->element_free) {
        list->element_free(&(node_to_remove->element));
    }

    free(node_to_remove);

    return list;
}

int dpl_size(dplist_t *list) {
    if (list == NULL) return -1;

    int count = 0;
    dplist_node_t *cursor = list->head;
    while (cursor != NULL) {
        count++;
        cursor = cursor->next;
    }
    return count;
}

void *dpl_get_element_at_index(dplist_t *list, int index) {
    if (list == NULL || list->head == NULL) return NULL;

    dplist_node_t *node;

    if (index <= 0) {
        node = list->head;
    } else {
        node = list->head;
        int i = 0;
        while (node->next != NULL && i < index) {
            node = node->next;
            i++;
        }
    }

    return node->element;
}

int dpl_get_index_of_element(dplist_t *list, void *element) {
    if (list == NULL || list->head == NULL) return -1;

    dplist_node_t *cursor = list->head;
    int index = 0;

    while (cursor != NULL) {
        // Use the compare callback if there is compare; if not, strict pointer comparison
        if (list->element_compare) {
            if (list->element_compare(cursor->element, element) == 0) {
                return index;
            }
        } else {
            if (cursor->element == element) {
                return index;
            }
        }
        cursor = cursor->next;
        index++;
    }

    return -1;
}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
    if (list == NULL || list->head == NULL) return NULL;

    dplist_node_t *node = list->head;

    if (index <= 0) {
        return node;
    }

    int i = 0;
    while (node->next != NULL && i < index) {
        node = node->next;
        i++;
    }
    return node;
}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {
    if (list == NULL || list->head == NULL || reference == NULL) return NULL;


    dplist_node_t *cursor = list->head;
    while (cursor != NULL) {
        if (cursor == reference) {
            return cursor->element;
        }
        cursor = cursor->next;
    }

    return NULL;
}