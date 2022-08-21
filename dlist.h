#ifndef RTOS_DLIST_H
#define RTOS_DLIST_H

#include "base.h"

BEGIN_EXTERN_C

struct DList;
struct DNode;

inline void init_dlist(struct DList* list);
inline void init_dnode(struct DNode* node);
inline int is_dlist_empty(struct DList* list);
void splice_dlist(struct DNode* dest, struct DNode* begin, struct DNode* end);
inline void splice_dnode(struct DNode* dest, struct DNode* source);
inline void remove_dnode(struct DNode* node);
void swap_dlist(struct DList* a, struct DList* b);

END_EXTERN_C

#endif  // RTOS_DLIST_H
