/* Functions for allocating / deallocating constant sized objects for use in realtime context */
#include "fastcache.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct fastcache_t {
    uint32_t cache_nblocks;
    /* Mask indicating which items are available. There are up to cache_size items. */
    uint32_t *cache_mask;
    /* Address of beginning of cache */
    char *cache_begin;
    /* Size of 1 item of cache as power of 2 */
    size_t cache_item_size;
};

void
fastcache_fprint(fastcache_t *fc, FILE *f)
{
    fprintf(f,
            "nblocks: %u\n", fc->cache_nblocks);
    fprintf(f,
            "mask: ");
    int i;
    for (i = 0; i < fc->cache_nblocks; i++) { 
        fprintf(f,"%#x ",fc->cache_mask[i]); 
    }
    fprintf(f,"\n");
    fprintf(f,"beg: %p\n",fc->cache_begin);
    fprintf(f,"itemsize: %zu\n",fc->cache_item_size);
}

void *
fastcache_alloc(fastcache_t *fc)
{
    uint32_t cache_idx = 0;
    uint32_t cache_block = 0;
    uint32_t *cbptr = fc->cache_mask;
    while ((cache_block < fc->cache_nblocks) && ((1 << cache_idx) & *cbptr) == 0) {
        cache_idx = (cache_idx + 1) & 0x1f;
        if (cache_idx == 0) {
            cache_block++;
            cbptr++;
        }
    }
    /*
    fastcache_fprint(fc,stderr);
    fprintf(stderr,"cache_block: %u\n",cache_block);
    */
    if (cache_block >= fc->cache_nblocks) {
        return NULL;
    }
    *cbptr &= ~(1 << cache_idx);
    return (void*)(fc->cache_begin +
        ((cache_idx + (cache_block << 5)) << fc->cache_item_size));
}

void
fastcache_free(fastcache_t *fc, void *ptr)
{
    uint32_t obj_idx = (((char*)ptr - fc->cache_begin) >> fc->cache_item_size);
    uint32_t cache_idx = obj_idx & 0x1f;
    uint32_t cache_block = obj_idx >> 5; 
    fc->cache_mask[cache_block] |= (1 << cache_idx);
}

/* 
   obj_size is size of object, this will be rounded up to next power of 2
   nobjs is number of objects, this will be rounded up to multiple of 32
*/
fastcache_t *
fastcache_new(size_t obj_size, size_t nobjs)
{
    if (obj_size == 0 || nobjs == 0) { return NULL; }
    size_t cache_item_size = 1;
    while ((1 << cache_item_size) < obj_size) { cache_item_size++; }
    nobjs = (nobjs + 31) & ~0x1f;
    fastcache_t *ret = calloc(sizeof(fastcache_t)
            +sizeof(uint32_t)*(nobjs >> 5) /* space for cache mask */
            +(1 << cache_item_size)*nobjs, /* space for cache items */
            1); 
    if (!ret) { return ret; }
    ret->cache_mask = (uint32_t*)(ret+1);
    ret->cache_begin = (char*)(ret->cache_mask + sizeof(uint32_t)*(nobjs >> 5));
    ret->cache_nblocks = (nobjs >> 5);
    ret->cache_item_size = cache_item_size;
    memset(ret->cache_mask,0xff,sizeof(uint32_t)*(nobjs >> 5));
    return ret;
}
