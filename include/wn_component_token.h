#ifndef WN_COMPONENT_TOKEN_H
#define WN_COMPONENT_TOKEN_H

#include <stdint.h>

#define WEB_NODE_TEMPLATE_TOKENS \
X(ID,'i','d') \
X(JS_DOM, 'd','E')\
X(GLOB,':',':') \
X(CL,'c','l')

enum {
    #define X(T,_,__) WN_TOKEN_INDEX_##T,
    WEB_NODE_TEMPLATE_TOKENS
    #undef X
    WEB_NODE_TEMPLATE_TOKENS_COUNT
};

typedef enum {
    #define X(T,_,__) WN_TOKEN_##T = 1 << WN_TOKEN_INDEX_##T,
    WEB_NODE_TEMPLATE_TOKENS
    #undef X
} wn_token_t;

#define tok16from8(c1,c2) ((uint16_t)((uint8_t)(c1) << 8 | (uint8_t)(c2)))

enum {
    #define X(T,c1,c2) TK16_##T = tok16from8(c1,c2),
    WEB_NODE_TEMPLATE_TOKENS
    #undef X
};

uint16_t tok16(const char* tok);

//looks for '...{token}...', applies (in order) token_func to token, rest_func to (...).
wn_token_t template_token_parse(const char* str, wn_token_t (*token_func)(const char* tok, void* cookie), wn_token_t (*rest_func)(const char* str, void* cookie), void* cookie);

//can define parentheses used for template_token_parse
wn_token_t template_token_parse_v(const char* str, wn_token_t (*token_func)(const char* tok, void* cookie), wn_token_t (*rest_func)(const char* str, void* cookie), void* cookie, const char* tok_start, const char* tok_end);

wn_token_t template_token_count(const char* str);
wn_token_t template_token_count_v(const char* str, const char* tok_start, const char* tok_end );

#endif