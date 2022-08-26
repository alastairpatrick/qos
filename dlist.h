#ifndef QOS_DLIST_H
#define QOS_DLIST_H

#include "base.h"
#include <stdbool.h>

QOS_BEGIN_EXTERN_C

typedef struct qos_dnode_t {
  struct qos_dnode_t* next;
  struct qos_dnode_t* prev;
} qos_dnode_t;

typedef struct qos_dlist_t {
  qos_dnode_t sentinel;
} qos_dlist_t;

void qos_splice_dlist(struct qos_dnode_t* dest, struct qos_dnode_t* begin, struct qos_dnode_t* end);
void qos_swap_dlist(struct qos_dlist_t* a, struct qos_dlist_t* b);

inline void qos_init_dnode(struct qos_dnode_t* node) {
  node->next = node;
  node->prev = node;
}

inline void qos_init_dlist(struct qos_dlist_t* list) {
  qos_init_dnode(&list->sentinel);
}

inline bool qos_is_dlist_empty(qos_dlist_t* list) {
  return list->sentinel.next == &list->sentinel;
}

inline void qos_splice_dnode(qos_dnode_t* dest, qos_dnode_t* source) {
  qos_dnode_t* a = source->prev;
  qos_dnode_t* b = source;
  qos_dnode_t* c = source->next;

  // Remove from source.
  a->next = c;
  c->prev = a;

  // Insert before dest.
  b->prev = dest->prev;
  b->next = dest;
  dest->prev->next = b;
  dest->prev = b;
}

inline void qos_remove_dnode(struct qos_dnode_t* node) {
  node->next->prev = node->prev;
  node->prev->next = node->next;
  node->next = node;
  node->prev = node;
}

QOS_END_EXTERN_C

#endif  // QOS_DLIST_H
