#ifndef RTOS_DLIST_IT_H
#define RTOS_DLIST_IT_H

#include "dlist.h"
#include "dlist.inl.c"
#include "dlist.struct.h"

#include <iterator>

template <typename T>
struct DListIterator {
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type   = std::ptrdiff_t;
  using value_type        = T;
  using pointer           = T*;
  using reference         = T&;
    
  explicit DListIterator(T& item): node(&item.node) {}
  explicit DListIterator(DNode* node): node(node) {}

  static inline DListIterator<T> begin(DList& list) {
    return DListIterator<T>(list.sentinel.next);
  }

  static inline DListIterator<T> end(DList& list) {
    return DListIterator<T>(&list.sentinel);
  }

  reference operator*() {
    return *(T*) node;
  }

  pointer operator->() {
    return (T*) node;
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

  DNode* node;
};

template <typename T>
static inline void splice(DListIterator<T> dest, DListIterator<T> begin, DListIterator<T> end) {
  splice_dlist(dest.node, begin.node, end.node);
}

template <typename T>
static inline void splice(DListIterator<T> dest, DListIterator<T> source) {
  splice_dnode(dest.node, source.node);
}

template <typename T>
static inline void splice(DListIterator<T> dest, T& source) {
  splice_dnode(dest.node, &source.node);
}

#endif  // RTOS_DLIST_IT_H
