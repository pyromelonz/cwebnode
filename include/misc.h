#ifndef MISC_H
#define MISC_H

#include <stddef.h>

#include <stdint.h>
#include "config.h"
#include "types.h"
#include "mem_dbg.h"

typedef enum ret_t {
    WS_FAILURE,
    WS_SUCCESS
} ret_t;

void scramble(void* a, uint32_t key, uint32_t len) ;
void unscramble(void* a, uint32_t key, uint32_t len) ;

char* file2str(const char* path);

//format must be "key key key" anything else is undefined
//allocates strings of post values into char** parameters after and corresponding to format
unsigned read_post_data(const char* post_data, const char* fmt, ...);


typedef struct {
    void* data,* it;
    size_t size;
} arena_t;

arena_t* new_arena(size_t cap);
void delete_arena(arena_t* a);

void* arena_malloc(arena_t* a, size_t size);

#endif //MISC_H
