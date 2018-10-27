#include "fastcache.h"
#include <assert.h>
#include <stdio.h>

#define OBJSZ 32
#define NOBJS 1024

int main (void)
{
    fastcache_t *fc = fastcache_new(OBJSZ,NOBJS);
    int i;
    char *addr = NULL;
    for (i = 0; i < NOBJS; i++) {
        char *tmp = fastcache_alloc(fc);
        if (!addr) {
            addr = tmp;
        }
        if (((tmp-addr)/OBJSZ) != i) {
            fprintf(stderr,"objs offset: %zu, i: %d\n",(tmp-addr)/OBJSZ,i);
        }
    }
    for (i = 0; i < NOBJS; i++) {
        char *tmp = addr+(OBJSZ*i);
        if (((tmp-addr)/OBJSZ) != i) {
            fprintf(stderr,"free objs offset: %zu, i: %d\n",(tmp-addr)/OBJSZ,i);
        }
        fastcache_free(fc,tmp);
    }
    for (i = 0; i < NOBJS; i++) {
        char *tmp = fastcache_alloc(fc);
        if (!addr) {
            addr = tmp;
        }
        if (((tmp-addr)/OBJSZ) != i) {
            fprintf(stderr,"2 objs offset: %zu, i: %d\n",(tmp-addr)/OBJSZ,i);
        }
    }

    return 0;
}

