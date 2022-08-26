#ifndef QOS_DLIST_H
#define QOS_DLIST_H

#include "base.h"
#include <stdbool.h>

BEGIN_EXTERN_C

typedef struct DNode {
  struct DNode* next;
  struct DNode* prev;
} DNode;

typedef struct DList {
  DNode sentinel;
} DList;

void splice_dlist(struct DNode* dest, struct DNode* begin, struct DNode* end);
void swap_dlist(struct DList* a, struct DList* b);

inline void init_dnode(struct DNode* node) {
  node->next = node;
  node->prev = node;
}

inline void init_dlist(struct DList* list) {
  init_dnode(&list->sentinel);
}

inline bool is_dlist_empty(DList* list) {
  return list->sentinel.next == &list->sentinel;
}

inline void splice_dnode(DNode* dest, DNode* source) {
  DNode* a = source->prev;
  DNode* b = source;
  DNode* c = source->next;

  // Remove from source.
  a->next = c;
  c->prev = a;

  // Insert before dest.
  b->prev = dest->prev;
  b->next = dest;
  dest->prev->next = b;
  dest->prev = b;
}

inline void remove_dnode(struct DNode* node) {
  node->next->prev = node->prev;
  node->prev->next = node->next;
  node->next = node;
  node->prev = node;
}

END_EXTERN_C

#endif  // QOS_DLIST_H
