#ifndef QOS_DLIST_IT_H
#define QOS_DLIST_IT_H

#include "dlist.h"

#include <iterator>

template <typename T, qos_dnode_t T::*NODE>
struct qos_dlist_iterator {
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type   = std::ptrdiff_t;
  using value_type        = T;
  using pointer           = T*;
  using reference         = T&;

  static pointer node_to_object(qos_dnode_t* node) {
    uintptr_t node_offset = uintptr_t(&(pointer(nullptr)->*NODE));
    return pointer(uintptr_t(node) - node_offset);
  }

  explicit qos_dlist_iterator(T* object): node(&object->*NODE) {}

  static inline qos_dlist_iterator begin(qos_dlist_t& list) {
    return qos_dlist_iterator(list.sentinel.next);
  }

  static inline qos_dlist_iterator end(qos_dlist_t& list) {
    return qos_dlist_iterator(&list.sentinel);
  }

  reference operator*() {
    return *node_to_object(node);
  }

  pointer operator->() {
    return node_to_object(node);
  }

  qos_dlist_iterator& operator++() {
    node = node->next;
    return *this;
  }

  qos_dlist_iterator operator++(int) {
    qos_dlist_iterator t = *this;
    node = node->next;
    return t;
  }

  qos_dlist_iterator& operator--() {
    node = node->prev;
    return *this;
  }

  qos_dlist_iterator operator--(int) {
    qos_dlist_iterator t = *this;
    node = node->prev;
    return t;
  }

  friend bool operator==(const qos_dlist_iterator& a, const qos_dlist_iterator& b) {
    return a.node == b.node;
  }

  friend bool operator!=(const qos_dlist_iterator& a, const qos_dlist_iterator& b) {
    return a.node != b.node;
  }

  friend bool empty(qos_dlist_iterator it) {
    return it.node == it.node->next;
  }

  // Removes referenced node and returns pointer to next.
  friend qos_dlist_iterator remove(qos_dlist_iterator it) {
    qos_dlist_iterator t(it.node->next);
    qos_remove_dnode(it.node);
    return t;
  }

  friend inline void splice(qos_dlist_iterator dest, qos_dlist_iterator begin, qos_dlist_iterator end) {
    qos_splice_dlist(dest.node, begin.node, end.node);
  }

  friend inline void splice(qos_dlist_iterator dest, qos_dlist_iterator source) {
    qos_splice_dnode(dest.node, source.node);
  }

  friend inline void splice(qos_dlist_iterator dest, T* source) {
    qos_splice_dnode(dest.node, &(source->*NODE));
  }

private:
  qos_dnode_t* node;
  explicit qos_dlist_iterator(qos_dnode_t* node): node(node) {}
};

#endif  // QOS_DLIST_IT_H
