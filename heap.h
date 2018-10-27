#ifndef HEAP_H
#define HEAP_H 

#include <stddef.h> 
#include <stdlib.h> 

/*
Allocate a minheap on the stack
varname is name of pointer to Heap
*/
#define HEAP_ALLOC(varname,maxsize,cmp,set_idx) \
    char varname ## _data__[sizeof(Heap) + sizeof(void*) * maxsize]; \
    varname = (Heap*)varname ## _data__; \
    Heap_init(varname, maxsize, cmp, set_idx)

typedef enum {
    HEAP_ENONE = 0,
    HEAP_ESIZE,
    HEAP_EEMPTY,
    HEAP_EINVAL,
    HEAP_EFULL
} HeapErr;

typedef struct Heap {
    size_t maxsize;
    size_t size;
    int (*cmp)(void*,void*);
    void (*set_idx)(void*,size_t);
    void *A[1];
} Heap;

void Heap_init(Heap *H,
               size_t maxsize,
               int (*cmp)(void*,void*),
               void (*set_idx)(void*,size_t));
Heap *Heap_new(size_t maxsize,
               int (*cmp)(void*,void*),
               void (*set_idx)(void*,size_t));
void Heap_heapify(Heap *H, size_t i);
HeapErr Heap_make_heap(Heap *H, void **A, size_t size);
HeapErr Heap_pop(Heap *H, void **key);
HeapErr Heap_push(Heap *H, void *key);
HeapErr Heap_top(Heap *H, void **key);
void Heap_clear(Heap *h);
static inline void Heap_free(Heap *h) { free(h); }
size_t Heap_size(Heap *H);

#endif /* HEAP_H */
