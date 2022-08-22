#ifndef RTOS_DLIST_IT_H
#define RTOS_DLIST_IT_H

#include "dlist.h"
#include "dlist.inl.c"
#include "dlist.struct.h"

#include <iterator>

template <typename T, DNode T::*NODE>
struct DListIterator {
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type   = std::ptrdiff_t;
  using value_type        = T;
  using pointer           = T*;
  using reference         = T&;

  pointer node_to_object(DNode* node) {
    uintptr_t node_offset = uintptr_t(&(pointer(nullptr)->*NODE));
    return pointer(uintptr_t(node) - node_offset);
  }

  DNode* object_to_node(pointer object) {
    return &(object->*NODE);
  }

  explicit DListIterator(T& item): node(object_to_node(&item)) {}
  explicit DListIterator(DNode* node): node(node) {}

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

  bool empty() {
    return node == node->next;
  }

  // Removes referenced node and returns pointer to next.
  DListIterator remove() {
    DListIterator t(node->next);
    remove_dnode(node);
    return t;
  }

  DNode* node;
};

template <typename T, DNode T::*NODE>
static inline void splice(DListIterator<T, NODE> dest, DListIterator<T, NODE> begin, DListIterator<T, NODE> end) {
  splice_dlist(dest.node, begin.node, end.node);
}

template <typename T, DNode T::*NODE>
static inline void splice(DListIterator<T, NODE> dest, DListIterator<T, NODE> source) {
  splice_dnode(dest.node, source.node);
}

template <typename T, DNode T::*NODE>
static inline void splice(DListIterator<T, NODE> dest, T& source) {
  splice_dnode(dest.node, &(source.*NODE));
}

#endif  // RTOS_DLIST_IT_H
