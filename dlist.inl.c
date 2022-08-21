#ifndef RTOS_DLIST_INL_C
#define RTOS_DLIST_INL_C

#include "dlist.struct.h"

inline void init_dlist(struct DList* list) {
  init_dnode(&list->sentinel);
}

inline void init_dnode(struct DNode* node) {
  node->next = node;
  node->prev = node;
}

inline int is_dlist_empty(DList* list) {
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

#endif  // RTOS_DLIST_INL_C
