#ifndef QOS_DLIST_IT_H
#define QOS_DLIST_IT_H

#include "dlist.h"

#include <iterator>

template <typename T, DNode T::*NODE>
struct DListIterator {
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type   = std::ptrdiff_t;
  using value_type        = T;
  using pointer           = T*;
  using reference         = T&;

  static pointer node_to_object(DNode* node) {
    uintptr_t node_offset = uintptr_t(&(pointer(nullptr)->*NODE));
    return pointer(uintptr_t(node) - node_offset);
  }

  explicit DListIterator(T* object): node(&object->*NODE) {}

  static inline DListIterator begin(DList& list) {
    return DListIterator(list.sentinel.next);
  }

  static inline DListIterator end(DList& list) {
    return DListIterator(&list.sentinel);
  }

  reference operator*() {
    return *node_to_object(node);
  }

  pointer operator->() {
    return node_to_object(node);
  }

  DListIterator& operator++() {
    node = node->next;
    return *this;
  }

  DListIterator operator++(int) {
    DListIterator t = *this;
    node = node->next;
    return t;
  }

  DListIterator& operator--() {
    node = node->prev;
    return *this;
  }

  DListIterator operator--(int) {
    DListIterator t = *this;
    node = node->prev;
    return t;
  }

  friend bool operator==(const DListIterator& a, const DListIterator& b) {
    return a.node == b.node;
  }

  friend bool operator!=(const DListIterator& a, const DListIterator& b) {
    return a.node != b.node;
  }

  friend bool empty(DListIterator it) {
    return it.node == it.node->next;
  }

  // Removes referenced node and returns pointer to next.
  friend DListIterator remove(DListIterator it) {
    DListIterator t(it.node->next);
    remove_dnode(it.node);
    return t;
  }

  friend inline void splice(DListIterator dest, DListIterator begin, DListIterator end) {
    splice_dlist(dest.node, begin.node, end.node);
  }

  friend inline void splice(DListIterator dest, DListIterator source) {
    splice_dnode(dest.node, source.node);
  }

  friend inline void splice(DListIterator dest, T* source) {
    splice_dnode(dest.node, &(source->*NODE));
  }

private:
  DNode* node;
  explicit DListIterator(DNode* node): node(node) {}
};

#endif  // QOS_DLIST_IT_H
