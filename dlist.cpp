#include "dlist.h"

#include <cassert>

void STRIPED_RAM qos_splice_dlist(qos_dnode_t* dest, qos_dnode_t* begin, qos_dnode_t* end) {
  if (begin == end) {
    return;
  }
  
  qos_dnode_t* a = begin->prev;
  qos_dnode_t* b = begin;
  qos_dnode_t* c = end->prev;
  qos_dnode_t* d = end;

  // Remove from source.
  a->next = d;
  d->prev = a;

  // Insert before dest.
  b->prev = dest->prev;
  c->next = dest;
  dest->prev->next = b;
  dest->prev = c;
}

void STRIPED_RAM qos_swap_dlist(qos_dlist_t* a, qos_dlist_t* b) {
  qos_dlist_t temp;

  qos_init_dlist(&temp);
  qos_splice_dlist(&temp.sentinel, a->sentinel.next, &a->sentinel);
  qos_splice_dlist(&a->sentinel, b->sentinel.next, &b->sentinel);
  qos_splice_dlist(&b->sentinel, temp.sentinel.next, &temp.sentinel);
  assert(qos_is_dlist_empty(&temp));
}
