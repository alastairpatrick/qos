#include "dlist.h"
#include "dlist.internal.h"
#include "dlist.inl.c"

#include <cassert>

void STRIPED_RAM splice_dlist(DNode* dest, DNode* begin, DNode* end) {
  if (begin == end) {
    return;
  }
  
  DNode* a = begin->prev;
  DNode* b = begin;
  DNode* c = end->prev;
  DNode* d = end;

  // Remove from source.
  a->next = d;
  d->prev = a;

  // Insert before dest.
  b->prev = dest->prev;
  c->next = dest;
  dest->prev->next = b;
  dest->prev = c;
}

void STRIPED_RAM swap_dlist(DList* a, DList* b) {
  DList temp;

  init_dlist(&temp);
  splice_dlist(&temp.sentinel, a->sentinel.next, &a->sentinel);
  splice_dlist(&a->sentinel, b->sentinel.next, &b->sentinel);
  splice_dlist(&b->sentinel, temp.sentinel.next, &temp.sentinel);
  assert(is_dlist_empty(&temp));
}
