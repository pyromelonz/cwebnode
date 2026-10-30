#include "container.h"
#include <stdint.h>
#include "misc.h"

/* STACK */

stk_t stack_push(stk_t i, void* d) {
    stk_t head = sw_malloc(sizeof(_stack_el));
    head->data = d;
    head->next = i;
    return head;
}

void* stack_pop(stk_t* i) {
    void* ret = 0;
    if (i && *i)  {
        stk_t tmp = *i;
        *i = (*i)->next;
        ret = tmp->data;
        sw_free(tmp);
    }
    return ret;
}

#include <stdarg.h>
#ifdef DEBUG
    #include "log.h"
#endif

unsigned stack_count(stk_t st) {
    unsigned ret = 0;
    while (st) {
        ret++;
        st = st->next;
    }
    return ret;
}

stk_t stack_from_variadic(void* d, ...) {
    va_list args;
    va_start(args,d);
    stk_t ret = 0;
    while (d) {
        ret = stack_push(ret,d);
        d = va_arg(args,void*);
    }
    va_end(args);
    return ret;
}

/* DEQUEUE */

dq_t* new_dq(data_deletion_func_t ddf) {
    dq_t* dq = sw_calloc(sizeof(dq_t),1);
    dq->del = ddf;
    return dq;
}

void dlt_dq(dq_t* dq) {
    void* data;
    while (dq->del && (data = dq_pop_f(dq))) dq->del(data);
    sw_free(dq);
}

void stack_flip(stk_t* i, stk_t* o) {
    void* tmp;
    while (tmp = stack_pop(i)) *o = stack_push(*o,tmp);
}

stk_t stk_flip(stk_t in) {
    stk_t out = 0;
    stack_flip(&in, &out);
    return out;
}

void dq_push_f(dq_t* dq, void* dt) {
    dq->f = stack_push(dq->f,dt);
}
void* dq_pop_f(dq_t* dq) {
    if (!dq->f) stack_flip(&dq->b, &dq->f);
    return stack_pop(&dq->f);
}

void dq_push_b(dq_t* dq, void* dt) {
    dq->b = stack_push(dq->b,dt);
}
void* dq_pop_b(dq_t* dq) {
    if (!dq->b) stack_flip(&dq->f, &dq->b);
    return stack_pop(&dq->b);
}

void dq_filter(dq_t* dq, int(*filter)(void*)) {
    if (!filter) return;
    stack_flip(&dq->f, &dq->b);
    void* tmp;

    while (tmp = dq_pop_b(dq)) {
        if (filter(tmp)) {
            dq->f = stack_push(dq->f,tmp);
        } else {
            dq->del(tmp);
        }
    }
}

/* BITSET */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

bs_t* new_bs(uint32_t n_bits) {
    bs_t* bs = sw_malloc(sizeof(bs_t));
    bs->n_chunks = (n_bits + 31) /32;
    bs->data = sw_calloc(sizeof(uint32_t), bs->n_chunks);
    return bs;
}

bs_t* new_bs_chunk(uint32_t c) {
    bs_t* bs = sw_malloc(sizeof(bs_t));
    bs->n_chunks = 1;
    bs->data = sw_calloc(sizeof(uint32_t), bs->n_chunks);
    bs->data[0] = c;
    return bs;
}

void bs_set(bs_t* bs, uint32_t bit) {
    if (bit >= (bs->n_chunks * 32)) {
        uint32_t new_n_chunks = ((bit + 32) /32);
        bs->data = sw_realloc(bs->data, sizeof(uint32_t) * new_n_chunks);
        memset(bs->data + (bs->n_chunks), 0, sizeof(uint32_t) * (new_n_chunks - bs->n_chunks));
        bs->n_chunks = new_n_chunks;
    }
    bs->data[bit / 32] |= (1U << (bit & 31));
}

void bs_clear(bs_t* bs, uint32_t bit) {
    if (bit/32 < bs->n_chunks) {
        bs->data[bit / 32] &= ~(1U << (bit & 31));
    }
    {
        unsigned n_remove = 0;
        while (bs->n_chunks && !bs->data[bs->n_chunks - 1]) {
            n_remove++;
        }
        bs->n_chunks -= n_remove;
        if (n_remove)
            bs->data = sw_realloc(bs->data, sizeof(uint32_t) * bs->n_chunks);
    }
}

int bs_get(const bs_t* bs, uint32_t bit) {
    return (bit < (bs->n_chunks * 32)) && (bs->data[bit / 32] & (1U << (bit & 31)));
}
int bs_any(const bs_t* bs) {
    int ret = 0;
    for (uint32_t c = 0; !ret && c < bs->n_chunks; c++) ret |= bs->data[c];
    return ret;
}
int bs_all(const bs_t* bs) {
    int ret = 1;
    for (uint32_t c = 0; ret && c < bs->n_chunks; c++) ret = (bs->data[c] == 0xFFFFFFFF);
    return ret;
}

size_t bs_bitindex(const bs_t* bs, size_t bit) {
    uint32_t acc = 0;
    unsigned i = 0;
    while( i<bs->n_chunks && (i<(bit/32))) {
        acc += __builtin_popcountll(bs->data[i++]);
    }
    return acc + ((i<bs->n_chunks) * __builtin_popcountll(bs->data[i] & ((1ULL << (bit & 31))-1)));
}

void bs_foreach(bs_t* bs, void(*func)(size_t bit, void* cookie), void* cookie) {

    for (size_t c = 0; c<bs->n_chunks; c++) {
        uint32_t ch = bs->data[c];
        size_t i = 0;
        while (ch) {
            if (1 & ch)
                func(c * 32 + i,cookie);
            i++;
            ch >>= 1;
        }
    }
}

void delete_bs(bs_t* bs) {
    sw_free(bs->data);
    sw_free(bs);
}

bs_t* new_bs_merge(bs_t* acc, bs_t* in) {
    if (acc && in); else return 0;

    uint32_t chunks = acc->n_chunks > in->n_chunks ? acc->n_chunks : in->n_chunks;

    bs_t* bs = sw_malloc(sizeof(bs_t));
    bs->n_chunks = chunks;
    bs->data = sw_calloc(chunks,sizeof(uint32_t));
    for (unsigned i = 0; i<chunks; i++) {
        bs->data[i] = ((acc->n_chunks < i) * acc->data[i]) | ((in->n_chunks < i) * in->data[i]);
    }
    return bs;
}
