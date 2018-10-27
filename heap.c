/*
 * A generalized heap
 */

#include "heap.h" 

#include <stddef.h> 
#include <stdio.h> 
#include <string.h> 

/* index of parent */
#define PARENT(i) (((size_t)i + 1)/2 - 1)

/* index of left child */
#define LEFT(i) (2*(size_t)i+1)

/* index of right child */
#define RIGHT(i) (LEFT(i)+1) 

/* swap keys */
#define SWAP(h,a,i_a,b,i_b) \
    void *__tmp; \
    __tmp = a; \
    a = b; \
    h->set_idx(a,i_a); \
    b = __tmp; \
    h->set_idx(b,i_b)

void Heap_init(Heap *H,
               size_t maxsize,
               int (*cmp)(void*,void*),
               void (*set_idx)(void*,size_t))
{
    H->maxsize = maxsize;
    H->size = 0;
    H->cmp = cmp;
    H->set_idx = set_idx;
}

Heap *Heap_new(size_t maxsize,
               int (*cmp)(void*,void*),
               void (*set_idx)(void*,size_t))
{
    Heap *h = (Heap*)calloc(1,sizeof(Heap)+sizeof(void*)*maxsize);
    if (h) {
        Heap_init(h,maxsize,cmp,set_idx);
    }
    return h;
}

void Heap_heapify(Heap *H, size_t i)
{
    while (LEFT(i) < H->size) {
        size_t min;
        if (H->cmp(H->A[i],H->A[LEFT(i)])) {
            min = LEFT(i);
        } else {
            min = i;
        }
        if ((RIGHT(i) < H->size) &&
                (H->cmp(H->A[min],H->A[RIGHT(i)]))) {
            min = RIGHT(i);
        }
        if (min == i) {
            break;
        }
        SWAP(H, H->A[i], i, H->A[min], min);
        i = min;
    }
}

/* Make a heap from contiguous array A.
 * If the items in A are to store their own index in the resulting heap, these
 * items should have their indices intialized to their index in the original A.
 * E.g, assuming A's actual type has an index field "idx", before Heap_make_heap
 * is called it should be that ((Type*)A[0])->idx = 0, ((Type*)A[1])->idx = 1, etc;
 */
HeapErr Heap_make_heap(Heap *H, void **A, size_t size)
{
    if (size > H->maxsize) {
        return HEAP_ESIZE;
    }
    H->size = size;
    memcpy(H->A, A, sizeof(void*) * size);
    size_t i = size;
    while (i--) {
        Heap_heapify(H, i);
    }
    return HEAP_ENONE;
}

HeapErr Heap_pop(Heap *H, void **key)
{
    if (!H->size) {
        return HEAP_EEMPTY;
    }
    if (!key) {
        return HEAP_EINVAL;
    }
    *key = H->A[0];
    H->set_idx(*key,H->maxsize);
    H->A[0] = H->A[H->size - 1];
    H->A[H->size - 1] = NULL;
    H->size--;
    Heap_heapify(H, 0);
    return HEAP_ENONE;
}

static void _float_up_last(Heap *H)
{
    size_t i = H->size - 1;
    while ((i > 0) && (H->cmp(H->A[PARENT(i)],H->A[i]))) {
        SWAP(H, H->A[PARENT(i)], PARENT(i), H->A[i], i);
        i = PARENT(i);
    }
}

/* For this, the items need not have their index field initialized. It will be
 * initialized provided set_idx is a function that sets the item's index. */
HeapErr Heap_push(Heap *H, void *key)
{
    if (H->size == H->maxsize) {
        return HEAP_EFULL;
    }
    H->A[H->size] = key;
    H->set_idx(H->A[H->size],H->size);
    H->size++;
    _float_up_last(H);
    return HEAP_ENONE;
}

/* Clear out all items from the heap, without popping them. References of
 * course should be held elsewhere to dynamically allocated items so they can
 * be freed. */
void Heap_clear(Heap *h)
{
    memset(h->A,0,sizeof(void*)*h->maxsize);
}

/* Returns reference to top element. Element is not removed, use Heap_pop for
   that. */
HeapErr Heap_top(Heap *H, void **key)
{
    if (!key) {
        return HEAP_EINVAL;
    }
    *key = NULL;
    if (H->size > 0) {
        *key = H->A[0];
        return HEAP_ENONE;
    }
    return HEAP_ESIZE;
}

size_t
Heap_size(Heap *H)
{
    return H->size;
}
