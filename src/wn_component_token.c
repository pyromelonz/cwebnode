#include "wn_component_token.h"
#include "mem_dbg.h"
#include "string.h"

wn_token_t rettok(const char* tok, void* cookie) {
    switch (tok16(tok)) {
        #define X(t,_,__) case TK16_##t: return WN_TOKEN_##t;
        WEB_NODE_TEMPLATE_TOKENS
        #undef X
        default: return 0;
    }
}

wn_token_t template_token_count(const char* str) {
    return template_token_parse(str,rettok,0,0);
}

wn_token_t template_token_count_v(const char* str, const char* tok_start, const char* tok_end ) {
    return template_token_parse_v(str,rettok,0,0,tok_start,tok_end);
}

wn_token_t template_token_parse_v(const char* str, wn_token_t (*token_func)(const char* tok, void* cookie), wn_token_t (*rest_func)(const char* str, void* cookie), void* cookie, const char* tok_start, const char* tok_end) {
    if (!str) return -1;
    char* fmt_copy = sw_strdup(str), *rest = fmt_copy, *tok;
    wn_token_t acc = 0;

    while (tok = strstr(rest,tok_start)) {
        *tok = 0;
        if (rest_func)acc|=rest_func(rest,cookie);
        tok+=strlen(tok_start);
        rest=strstr(tok,tok_end);
        *rest=0;
        if (token_func)acc|=token_func(tok,cookie);
        rest+=strlen(tok_end);
    }

    if (rest_func)rest_func(rest,cookie);
    
    sw_free(fmt_copy);

    return acc;
}

wn_token_t template_token_parse(const char* str, wn_token_t (*token_func)(const char* tok, void* cookie), wn_token_t (*rest_func)(const char* str, void* cookie), void* cookie) {
    return template_token_parse_v(str,token_func,rest_func,cookie,"{","}");
}

uint16_t tok16(const char* tok) {
    return (tok && tok[0]) ? tok16from8(tok[0],tok[1]) : 0;
}