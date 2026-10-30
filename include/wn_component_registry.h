#ifndef WWW_COMPONENTS_H
#define WWW_COMPONENTS_H

#define ___STR2(x) #x 
#define ___STR(x) ___STR2(x)
#define AUTO_REG(s) comp_reg_set(__FILE__ ___STR(__LINE__),s,0)

#include <stddef.h>

#define _STATIC_WWW_COMPONENTS_LIST \
X(INVALID,"internal error") \
X(HTML_GLOB, "{::}") \
X(DISPLAY_INLINE_BLOCK_CHILDREN,"{c>}display: inline-block") \
X(DISPLAY_INLINE_BLOCK,"display: inline-block")

typedef enum {
    #define X(a,_) REG_##a,
    _STATIC_WWW_COMPONENTS_LIST
    #undef X
    _STATIC_WWW_COMPONENTS_COUNT,
    _STATIC_WWW_COMPONENTS_EXTRA
} static_reg_index_t;

size_t comp_reg_max_index();

extern const char* static_comp_reg[_STATIC_WWW_COMPONENTS_COUNT];

void comp_reg_init(), comp_reg_term();

void cr_block_rehash();
void cr_unblock_rehash();

const char* comp_reg_get(const char* key);

typedef size_t reg_ambindex;

const char* comp_reg_ambi_get(reg_ambindex i);
#ifdef DEBUG
void comp_reg_log_all();
#endif

enum COMP_REG_STORE_FLAGS {
    CRSTORE_FREEDATA = 1, CRSTORE_KEYDUP = 2
};

typedef struct {
    void* data;
    size_t index;
    enum COMP_REG_STORE_FLAGS flags;
    const char* key;
} entry_t;

//DO NOT ASSIGN NEW ENTRIES DURING BLOCK!!!!
entry_t* comp_reg_set(const char* key, void* data, enum COMP_REG_STORE_FLAGS);
entry_t* comp_reg_get_p(const char* key);

void* comp_reg_data(size_t index);


#endif //WWW_COMPONENTS_H