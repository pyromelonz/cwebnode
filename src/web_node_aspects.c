#include "web_node_macro.h"
#include "log.h"
#include <stdio.h>
#include "misc.h"
#include <string.h>

DEFINE_WNA_DESTRUCTOR(WNA_ROOT) {
    delete_bs(data->css_classes_bs);
    while (data->css_refs) sw_free(stack_pop(&data->css_refs));
    while (data->js_refs) sw_free(stack_pop(&data->js_refs));
}

p_sw_free(wna_css_reg);

DEFINE_WNA_INIT_FUNC(WNA_ROOT) {
    data->wnid = 0;
    data->css_refs = 0;
    data->js_refs = 0;
    data->css_classes_bs = new_bs(WNA_COUNT);
}

DEFINE_WNA_INIT_FUNC(WNA_CHILDREN) {
    if (init); else {data->subnodes = 0; return;};
    data->subnodes = init->subnodes;
    for (stk_t i = data->subnodes; i; i=i->next) {
        web_node* iwn = i->data;
        iwn->parent = wn;
    }
}

DEFINE_WNA_DESTRUCTOR(WNA_CHILDREN) {
    while (data->subnodes) {
        delete_web_node(stack_pop(&data->subnodes));
    }
}

wna_strrs_data_t* init_string_resource (wna_strrs_init_t* init) {
    wna_strrs_data_t* data = sw_malloc(sizeof(wna_strrs_data_t));
    
    switch (init->flags | (WNA_STRRS_TOPFLAG * (init->index < _STATIC_WWW_COMPONENTS_EXTRA)) ) {
        case 4:
        case 0: //just static pointer ref
            data->storage_method = WNA_STRRS_STORE_PTR_STATIC;
            data->access_data = (void*)init->value;
            break;

        case 6:
        case 2: //NO REG, DUP
            data->storage_method = WNA_STRRS_STORE_PTR_ALLOC;
            data->access_data = (void*)sw_strdup(init->value);
            break;

        case 3: case 1:  //DYNAMIC REGISTRY
        if (init->key) {  //dynamic reg
                data->storage_method = WNA_STRRS_STORE_REG_DYNAMIC;
                data->access_data = comp_reg_get_p(init->key);     
                if (init->value && (!data->access_data 
                    || (data->access_data && (!*(const char*)comp_reg_data(((entry_t*)data->access_data)->index) ))) //also replace comp reg data if previous is nullstr
                ) {
                    LOG(DBG,"new www component registry entry : %s, %s. Previously: %s",init->key, init->value, comp_reg_get(init->key));
                    data->access_data = comp_reg_set(init->key,
                        (void*)((init->flags & WNA_STRRS_STRDUP) ?sw_strdup(init->value) : init->value),
                        (!!(init->flags & WNA_STRRS_STRDUP)*CRSTORE_FREEDATA) | (CRSTORE_KEYDUP));
                    LOG(DBG,"new access_data ptr: %p, %u", data->access_data, *(size_t*)data->access_data);
                }
            } break;
        
        case 7: //REGISTRY DUP, STATIC -- invalid
            LOG(CRIT,"Can't allocate for static registry");
        case 5:
            data->storage_method = WNA_STRRS_STORE_REG_STATIC;
            data->access_data = (void*)init->index;
            break;
    }
    return data;
}

DEFINE_WNA_INIT_FUNC(WNA_STRING_RESOURCE) { //accesslist must be initialised to 0
    data->access_list = stack_push((stk_t)((!!wn->n_str_params++) * (uintptr_t)data->access_list),init_string_resource(init));
}

DEFINE_WNA_DESTRUCTOR(WNA_STRING_RESOURCE) {
    wna_strrs_data_t* tmp;
    while (tmp = stack_pop(&data->access_list)) {
        if (tmp->storage_method == WNA_STRRS_STORE_PTR_ALLOC) {
            sw_free(tmp->access_data);
        }
        sw_free(tmp);
    }
}

const char* get_strrs(web_node* wn, unsigned index) {
    struct _DATA_WNA_STRING_RESOURCE* wnadata = WNA_P_DATA(wn,WNA_STRING_RESOURCE);
    if (!wnadata) return 0;
    stk_t list = wnadata->access_list;
    while (list && index--) list = list->next;

    if (list) {
        wna_strrs_data_t* data = list->data;
        switch (data->storage_method) {
        case WNA_STRRS_STORE_REG_DYNAMIC:
            return comp_reg_data(((entry_t*)data->access_data)->index);
        case WNA_STRRS_STORE_REG_STATIC:
            return static_comp_reg[(size_t)data->access_data];
        case WNA_STRRS_STORE_PTR_ALLOC:
        case WNA_STRRS_STORE_PTR_STATIC:
            return (char*)data->access_data;
    }}
    return 0;
}

reg_ambindex get_strrs_reg_ambindex(web_node* wn, unsigned index) {
    
    struct _DATA_WNA_STRING_RESOURCE* wnadata = WNA_P_DATA(wn,WNA_STRING_RESOURCE);
    if (!wnadata) return 0;
    stk_t list = wnadata->access_list;
    while (list && index--) list = list->next;
    if (list) {wna_strrs_data_t* data = list->data;
        switch (data->storage_method) {
        case WNA_STRRS_STORE_REG_DYNAMIC:
            return ((entry_t*)data->access_data)->index + _STATIC_WWW_COMPONENTS_EXTRA;
        case WNA_STRRS_STORE_REG_STATIC:
            return (size_t)data->access_data;
        default:
            return _STATIC_WWW_COMPONENTS_COUNT;
    }}
    return REG_INVALID;
}

stk_t wn_get_children(web_node* wn) {
    return (wn->aspects & WNA_CHILDREN) ? WNA_P_DATA(wn,WNA_CHILDREN)->subnodes : 0;
}

DEFINE_WNA_ASPECT(WNA_CSS_CLASS) {
    struct _DATA_WNA_ROOT* rtd;
    {
        web_node* const root = wn_get_root(wn);
        if (!root) return;
        rtd = root ? WNA_P_DATA(root,WNA_ROOT) : 0;
    }

    data->clid = get_strrs_reg_ambindex(wn,0);
    bs_set(rtd->css_classes_bs,data->clid);

    web_node* html_target = wn->parent;
    if (html_target && (html_target->aspects & WNA_HTML_BODY)) {
        WNA_P_DATA(html_target,WNA_HTML_BODY)->flags |= WNA_HTML_CLASS_TARGET;
    } else {
        LOG(WARN,"CSS Class WNA could not find html applicant");
        return;
    }
}

#include <string.h>
#include <stdlib.h>

DEFINE_WNA_INIT_FUNC(WNA_HTML_BODY) {
    data->flags = 0;
    data->tag = init->tag;
    if (!(wn->aspects & WNA_STRING_RESOURCE)) LOG(CRIT,"HTML node missing string resource");
}

DEFINE_WNA_ASPECT(WNA_CSS_REF) {
    wnid_t id;

    web_node* html_target = wn->parent;
    if (html_target && (html_target->aspects & WNA_HTML_BODY)) {
        id = html_target->id;
    } else {
        LOG(WARN,"CSS reference WNA could not find html applicant");
        return;
    }

    WNA_P_DATA(html_target,WNA_HTML_BODY)->flags |= WNA_HTML_ID_TARGET;

    WNA_DATA_STRUCT(WNA_ROOT)* const rtd = WNA_P_DATA(wn_get_root(wn),WNA_ROOT);

    rs_delivery_t* idrs = sw_malloc(sizeof(rs_delivery_t));
    idrs->id = id;
    idrs->rs = wn;
    rtd->css_refs = stack_push(rtd->css_refs,idrs);
}

DEFINE_WNA_ASPECT(WNA_JS_REF) {
    wnid_t id;

    web_node* html_target = wn->parent;
    if (html_target && (html_target->aspects & WNA_HTML_BODY)) {
        id = html_target->id;
    } else {
        LOG(WARN,"JS reference WNA could not find html applicant");
        return;
    }

    WNA_P_DATA(html_target,WNA_HTML_BODY)->flags |= WNA_HTML_ID_TARGET;

    WNA_DATA_STRUCT(WNA_ROOT)* rtd = WNA_P_DATA(wn_get_root(wn),WNA_ROOT);
    rs_delivery_t* idrs = sw_malloc(sizeof(rs_delivery_t));
    idrs->id = id;
    idrs->rs = wn;
    rtd->js_refs = stack_push(rtd->js_refs,idrs);
}

#define X(a) _WNA_ASPECT_FUNC_NAME(a),
const void (*wna_aspect_funcs[])(dummy_handler*, web_node*) = {WEB_NODE_ASPECT_LIST};
#undef X
#define X(a) _WNA_DESTRUCTOR_NAME(a),
const void (*wna_destructors[])(void*) = {WEB_NODE_ASPECT_LIST};
#undef X
#define X(a) _WNA_INIT_FUNC_NAME(a),
const void (*wna_constructors[])(void*, void*, web_node*) = {WEB_NODE_ASPECT_LIST};
#undef X

#define X(a) sizeof(WNA_DATA_STRUCT(a)),
const size_t wna_struct_sizes[WNA_COUNT] = {WEB_NODE_ASPECT_LIST};
#undef X

#define X(a) _Alignof(WNA_DATA_STRUCT(a)),
const size_t wna_struct_alignments[WNA_COUNT] = {WEB_NODE_ASPECT_LIST};
#undef X

/* Each aspect has its own C alignment requirements. In particular,
 * CSS_REF is one byte while STRING_RESOURCE contains a pointer; simply
 * summing sizeof(struct) values misaligns the latter. */
static size_t wna_layout_size_to(wna_t aspects, wna_flag target) {
    size_t offset = 0;
    for (wna_index i = 0; i < WNA_COUNT; i++) {
        const wna_flag flag = wna_i2f(i);
        if (!(aspects & flag)) continue;
        const size_t align = wna_struct_alignments[i];
        offset = (offset + align - 1) & ~(align - 1);
        if (flag == target) return offset;
        offset += wna_struct_sizes[i];
    }
    return offset;
}

size_t wna_struct_offset(wna_t aspects, wna_flag aspect) {
    return wna_layout_size_to(aspects, aspect);
}

size_t wna_struct_size_total(wna_t aspects) {
    return wna_layout_size_to(aspects, 0);
}

void* wna_get_aspect_struct(web_node* wn, wna_flag aspect) {
    return ((wn->aspects & aspect) && wn->data) ? wn->data + wna_struct_offset(wn->aspects,aspect) : 0;
}