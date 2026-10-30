#ifndef WEB_NODE_ASPECTS_H
#define WEB_NODE_ASPECTS_H

#include "misc.h"
#include "web_node.h"

#define WNA_DATA_STRUCT(a) struct _DATA_##a
#define WNA_INIT_STRUCT(a) struct _INIT_##a

extern const void (*wna_aspect_funcs[])(dummy_handler*, web_node*);
extern const void (*wna_destructors[])(void*);
extern const void (*wna_constructors[])(void* data, void* init, web_node* wn);
extern const size_t wna_struct_sizes[WNA_COUNT];
extern const size_t wna_struct_alignments[WNA_COUNT];

#define HTML_TAGLIST \
X(plain) \
X(div) \
X(body) \
X(ol) \
X(ul) \
X(li) \
X(a) \
X(h1) \
X(form) \
X(label) \
X(input) \
X(button)

enum WNA_HTML_TAGS {
    #define X(a) TAG_##a,
    HTML_TAGLIST
    #undef X
};

size_t wna_struct_offset(wna_t aspects, wna_flag aspect);
size_t wna_struct_size_total(wna_t aspects);

void* wna_get_aspect_struct(web_node* wn, wna_flag aspect);

#define WNA_P_DATA(webnode,aspect) ((WNA_DATA_STRUCT(aspect)*)(wna_get_aspect_struct(webnode,aspect)))

enum WNA_HTML_ELEMENT_FLAGS {
    WNA_HTML_ID_TARGET = 1, WNA_HTML_CLASS_TARGET = 2
};

const char* get_strrs(web_node* wn, unsigned index);

typedef struct {
    uid_t id;
    web_node* rs;
} rs_delivery_t;

WNA_DATA_STRUCT(WNA_ROOT){
    unsigned wnid;
    stk_t css_refs, js_refs; //rs_delivery_t
    void* css_classes_bs;
};

WNA_DATA_STRUCT(WNA_CHILDREN) {
    stk_t subnodes;
};
WNA_INIT_STRUCT(WNA_CHILDREN) {
    stk_t subnodes;
};

stk_t wn_get_children(web_node* wn);

WNA_INIT_STRUCT(WNA_HTML_BODY) {
    enum WNA_HTML_TAGS tag;
};

WNA_DATA_STRUCT(WNA_HTML_BODY) {
    uint32_t flags;
    enum WNA_HTML_TAGS tag;
};

enum WNA_STRRS_INIT_FLAGS {WNA_STRRS_REGISTRY = 1, WNA_STRRS_STRDUP = 2, WNA_STRRS_TOPFLAG = 4};
enum WNA_STRRS_STORAGE_METHOD {WNA_STRRS_STORE_REG_DYNAMIC,WNA_STRRS_STORE_REG_STATIC, WNA_STRRS_STORE_PTR_ALLOC, WNA_STRRS_STORE_PTR_STATIC };

WNA_INIT_STRUCT(WNA_STRING_RESOURCE) {
    enum WNA_STRRS_INIT_FLAGS flags;
    static_reg_index_t index; 
    const char* key, *value; //if path or string or reggkey
};

typedef WNA_INIT_STRUCT(WNA_STRING_RESOURCE) wna_strrs_init_t;

typedef struct {
    void* access_data;
    enum WNA_STRRS_STORAGE_METHOD storage_method;
} wna_strrs_data_t;

WNA_DATA_STRUCT(WNA_STRING_RESOURCE) {
    stk_t access_list; //wna_strrs_data_t
};

WNA_DATA_STRUCT(WNA_CSS_CLASS){ uid_t clid; reg_ambindex class_rs ;};
WNA_DATA_STRUCT(WNA_CSS_REF) {unsigned char custom_selector;};
WNA_DATA_STRUCT(WNA_JS_REF) {};

WNA_INIT_STRUCT(WNA_CSS_CLASS){};
WNA_INIT_STRUCT(WNA_CSS_REF) {unsigned char custom_selector;};
WNA_INIT_STRUCT(WNA_JS_REF) {};


WNA_INIT_STRUCT(WNA_ROOT) {};

#endif