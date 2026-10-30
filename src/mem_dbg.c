
#include "mem_dbg.h"

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_MEM_V

#include <pthread.h>
#include "log.h"

#ifdef CONFIG_NUTTX
pthread_mutex_t allocs_lock = _PTHREAD_MUTEX_INITIALIZER;
#else
pthread_mutex_t allocs_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

#include "rb.h"

rbtree* allocs = 0;

typedef struct  {
    void* alloc;
    const char* file;
    int line;
}alloc_t;

int alloc_cmp(const void* _a, const void* _b) {
    const void* a = ((alloc_t*)_a)->alloc, *b = ((alloc_t*)_b)->alloc;
    return (a<b)-(b<a);
}

void* _check_free(void* p, const char* file, int line) {

    if (!p) {LOG(CRIT,"Trying to free nullptr from %s line %d",file,line); return 0;}
    alloc_t c;
    c.alloc = p;
    rbnode* found;
    
    if (allocs && (found = rb_find(allocs, &c))) {
        alloc_t* a = found->data;
        LOG(MEM,"Freeing %p from %s line %d, allocated from %s line %d",p,file,line, a->file, a->line);

        pthread_mutex_lock(&allocs_lock);
        rb_delete(allocs,found,0);
        pthread_mutex_unlock(&allocs_lock);
    } else {
        LOG(FATAL,"Double freeing %p from %s line %d!",p,file,line);
    }
    _sw_free(p);
    return 0;
}

void* register_alloc(void* alloc, size_t size, const char* file, int line) {
    alloc_t* e = malloc(sizeof(alloc_t));

    if (size)
        LOG(MEM,"Allocated %u bytes to %p from %s line %d", size, alloc, file, line);
    else 
        LOG(MEM,"Allocated string \"%s\" to %p from %s line %d", alloc, alloc, file,line);

    e->alloc = alloc;
    e->file = file;
    e->line = line;
    
    pthread_mutex_lock(&allocs_lock);
    rb_insert(allocs, e);
    pthread_mutex_unlock(&allocs_lock);

    return alloc;
}

void* register_alloc_init(void* alloc,size_t size,  const char* file, int line);

void* (*_register_alloc)(void*, size_t size, const char*, int) = register_alloc_init;

void* register_alloc_init(void* alloc, size_t size, const char* file, int line) {
    _register_alloc = register_alloc;
    pthread_mutex_lock(&allocs_lock);
    if (!allocs) allocs = rb_create(alloc_cmp,_sw_free);
    pthread_mutex_unlock(&allocs_lock);

    return register_alloc(alloc,size,file,line);
}

int print_remaining_alloc(void* n, void* c) {
    alloc_t* at = n;
    *(size_t*)c = *(size_t*)c+1;
    LOG(WARN,"Remaining allocation %p from file %s, line %d",at->alloc,at->file,at->line);
}

void mem_term() {
    size_t c = 0;
    RB_APPLY(allocs,print_remaining_alloc,&c,INORDER);
    LOG(DBG,"Remaining allocations: %u",c);
    rb_destroy(allocs);
}

void* __sw_realloc(void* p, size_t s) {
    LOG(DBG,"finding %p",p);
    alloc_t c;
    c.alloc = p;
    pthread_mutex_lock(&allocs_lock);
    rbnode* n;
    if (allocs && (n = rb_find(allocs,&c))) {
        alloc_t* a = n->data;
        rb_delete(allocs,n,1);
        p = (a->alloc = realloc(p,s));
        rb_insert(allocs,a);
    } else {
        LOG(CRIT,"Trying to reallocate invalid allocation");
    }
    pthread_mutex_unlock(&allocs_lock);
    return p;
}

void* (*_sw_realloc)(void*,size_t) = __sw_realloc;

#else

void* (*_sw_realloc)(void*,size_t) = realloc;

#endif

#if defined(DEBUG_MEM)
#ifdef DEBUG_MEM_V
    #error Mutual exclusion: DEBUG_MEM and DEBUG_MEM_V
#endif

static int allocs = 0;

void* __sw_malloc(size_t s) {
    allocs++;
    return malloc(s);
}
void* __sw_calloc(size_t s, size_t s2) {
    allocs++;
    return calloc(s,s2);
}
char* __sw_strdup(const char* s) {
    allocs++;
    return strdup(s);
}
void __sw_free(void* s) {
    allocs--;
    free(s);
}

void* (*_sw_calloc)( size_t, size_t) = __sw_calloc;
void* (*_sw_malloc) (size_t) = __sw_malloc;
char* (*_sw_strdup)(const char*) = __sw_strdup;
void (*_sw_free)(void* ptr) = __sw_free;

int mem_get_remaining_allocations() {
    return allocs;
}

#else

void* (*_sw_calloc)( size_t, size_t) = calloc;
void* (*_sw_malloc) (size_t) = malloc;
char* (*_sw_strdup)(const char*) = strdup;
void (*_sw_free)(void* ptr) = free;

#endif
