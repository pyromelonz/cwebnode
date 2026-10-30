#ifndef MEM_DBG_H
#define MEM_DBG_H

#include <stddef.h>
#include "config.h"

extern void* (*_sw_calloc)( size_t num, size_t size);
extern void* (*_sw_malloc) (size_t bytes);
extern char* (*_sw_strdup)(const char* str);
extern void (*_sw_free)(void* ptr);
extern void* (*_sw_realloc)(void* ptr, size_t size);

#define p_sw_free(name) void name##_sw_free(void* p) {sw_free(p);}

#ifdef DEBUG_MEM_V
    extern void* (*_register_alloc)(void* p, size_t size, const char* file, int line);
    void* _check_free(void* p, const char* file, int line);
    #define sw_malloc(b) _register_alloc(_sw_malloc(b),b,__FILE__,__LINE__)
    #define sw_calloc(n,s) _register_alloc(_sw_calloc(n,s),n*s,__FILE__,__LINE__)
    #define sw_strdup(s) _register_alloc((void*)_sw_strdup(s),0,__FILE__,__LINE__)
    #define sw_free(p) _check_free((void*)p,__FILE__,__LINE__)

    void mem_term();
#else
    #define sw_calloc _sw_calloc
    #define sw_malloc _sw_malloc
    #define sw_strdup _sw_strdup
    #define sw_free _sw_free
#endif

#ifdef DEBUG_MEM
    int mem_get_remaining_allocations();
#endif


    #define sw_realloc _sw_realloc

#endif //MEM_DBG_H