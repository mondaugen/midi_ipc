#ifndef FASTCACHE_H
#define FASTCACHE_H 

/* Functions for allocating / deallocating constant sized objects for use in realtime context */
#include <stddef.h>

typedef struct fastcache_t fastcache_t;

void *
fastcache_alloc(fastcache_t *fc);


void
fastcache_free(fastcache_t *fc, void *ptr);


/* 
   obj_size is size of object, this will be rounded up to next power of 2
   nobjs is number of objects, this will be rounded up to multiple of 32
*/
fastcache_t *
fastcache_new(size_t obj_size, size_t nobjs);

#endif /* FASTCACHE_H */
