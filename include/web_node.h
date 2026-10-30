#ifndef WEB_MISC_H
#define WEB_MISC_H

#include "webserver_api_functions.h"
#include "container.h"
#include "types.h"

#include "wn_component_registry.h"

// INITIALISED IN THIS ORDER
#define WEB_NODE_ASPECT_LIST \
X(WNA_HTML_BODY) \
X(WNA_CSS_CLASS) \
X(WNA_CSS_REF) \
X(WNA_JS_REF) \
X(WNA_CHILDREN) \
X(WNA_ROOT) \
X(WNA_STRING_RESOURCE)

typedef enum {
    WNA_NONE = -1,
    #define X(a) a##_NUM,
    WEB_NODE_ASPECT_LIST
    #undef X
    WNA_COUNT
} wna_index;

typedef enum {
    #define X(a) a = 1 << a##_NUM,
    WEB_NODE_ASPECT_LIST
    #undef X
} wna_flag;

wna_index wna_f2i (wna_flag f);
wna_flag wna_i2f (wna_index i);

typedef uint32_t wna_t;
typedef uint32_t wnid_t;

typedef struct web_node {
    void* data;
    wnid_t id;
    wna_t aspects;
    struct web_node* parent;
    uint8_t n_str_params;
} web_node;

web_node* wn_get_root(web_node* tree);

// CONVENTION: _new_web_node_ooo() [web_node_aspect type, void* aspect_init_struct,], (-1 === WNA_NONE));
web_node* _new_web_node_ooo(wna_flag aspect, ...);

//enter init structs of all required aspects
#define NEW_WN(...) _new_web_node_ooo(__VA_ARGS__,WNA_NONE)

unsigned char wn_check_aspect(web_node* wn, wna_flag a);

int delete_web_node(web_node* wn);

// void wn_apply_tree(web_node* tree, void (*func)(web_node*, void* c),void* cookie);

err_t wn_attach(web_node* parent, web_node* child);
web_node* wn_attach_more(web_node* parent, web_node* children, ...);
#define WN_ADD(parent, ...) wn_attach_more(parent, __VA_ARGS__, 0);

void wn_apply_all_aspects_to_tree(web_node* rt, dummy_handler* s);

err_t wn_deliver(dummy_handler*s, web_node* wn);

web_node* wn_wrap_html_root(web_node* wn);

web_node* wn_html_root();

/*
web_node* wn_user_header(dummy_handler* s);

//web_node* pop_wn_sub_html(web_node* wn, ...);

*/

#endif //WEB_MISC_H