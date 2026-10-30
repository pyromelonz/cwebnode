#ifndef CONTAINER_H
#define CONTAINER_H

#include <stdint.h>
#include <types.h>
#include <stddef.h>

/* Backend will be made array based soon*/


/* STACK */

typedef struct stack_el {
    void* data;
    struct stack_el* next;
} _stack_el;

typedef void (*data_deletion_func_t)(void*);

typedef _stack_el* stk_t;

typedef struct {
    stk_t f;
    stk_t b;
    data_deletion_func_t del;
} dq_t;

stk_t stack_push(stk_t i, void* d);
void* stack_pop(stk_t* i);

unsigned stack_count(stk_t st);
void stack_flip(stk_t* i, stk_t* o);

stk_t stk_flip(stk_t in);

//adds arguments to stack until nullptr encountered. Don't forget the nullptr.

stk_t stack_from_variadic(void* data, ...);
#define STACK_FROM_VARIADIC(...) stack_from_variadic(__VA_ARGS__,0)

/* DEQUEUE */

void dq_push_f(dq_t* dq, void* dt);
void* dq_pop_f(dq_t* dq);

void dq_push_b(dq_t* dq, void* dt);
void* dq_pop_b(dq_t* dq);

void dq_filter(dq_t* dq, int(*filter)(void*));

dq_t* new_dq(data_deletion_func_t ddf);
void dlt_dq(dq_t*);

/* BITSET */ 

typedef struct {
    uint32_t* data;
    uint32_t n_chunks;
} bs_t;

bs_t* new_bs(uint32_t n_bits);
bs_t* new_bs_chunk(uint32_t init_chunk);

bs_t* new_bs_merge(bs_t* acc, bs_t* in);
void delete_bs(bs_t* bs);

void bs_clear(bs_t* bs, uint32_t bit);
void bs_set(bs_t* bs, uint32_t bit);

size_t bs_bitindex(const bs_t* bs, size_t bit);

void bs_foreach(bs_t* bs, void(*func)(size_t bit, void* cookie), void* cookie);

int bs_get(const bs_t* bs, uint32_t bit);
int bs_any(const bs_t* bs);
int bs_all(const bs_t* bs);


#endif//CONTAINER_H