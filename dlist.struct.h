#ifndef RTOS_DLIST_STRUCT_H
#define RTOS_DLIST_STRUCT_H

typedef struct DNode {
  struct DNode* next;
  struct DNode* prev;
} DNode;

typedef struct DList {
  DNode sentinel;
} DList;

#endif  // RTOS_DLIST_STRUCT_H
