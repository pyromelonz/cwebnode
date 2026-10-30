#include "web_node_macro.h"
#include "misc.h"
#include <stdarg.h>
#include <stdlib.h>
#include "log.h"
#include "container.h"
#include <string.h>


#ifdef DEBUG
#define X(a) #a,
    static const char* WNA_ASPECT_STRINGS[] = {WEB_NODE_ASPECT_LIST};
#undef X
#endif



wna_index wna_f2i (wna_flag f) {
    return __builtin_ctz(f);
}

wna_flag wna_i2f (wna_index i ) {
    return 1<<i;
}

int delete_web_node(web_node* wn) {
    if (wn->data) {
            for (wna_index i = 0; i<WNA_COUNT; i++) {
                if (wn->aspects & (1<<i)) {
                    void(* ddf)(void*) = wna_destructors[i];
                    if (ddf) ddf(wn->data + wna_struct_offset(wn->aspects,1<<i));
                }
            }
            sw_free(wn->data);
        }
    sw_free(wn);
    

    return 0;
}

typedef struct {
    wna_flag aspect;
    void* init_struct;
} wna_init_t;

web_node* new_web_node(wna_t aspects, stk_t init_list) {

    web_node* wn = sw_malloc(sizeof(web_node));
    wn->aspects = aspects;
    wn->n_str_params = 0;
    wn->parent = 0;
    wn->id = 0;
    {
        size_t datasize = 0;
        for (wna_index i = 0; i<WNA_COUNT; i++) {
            datasize += !!(aspects & wna_i2f(i)) * wna_struct_sizes[i];
        }
        wn->data = datasize ? sw_malloc(datasize) : 0;
    }

    while (init_list) {
        wna_init_t* init = stack_pop(&init_list);
        const unsigned ctor_index = wna_f2i(init->aspect);
        if (aspects & init->aspect) {
            if (wna_constructors[ctor_index]) wna_constructors[ctor_index]((char*)wn->data + wna_struct_offset(wn->aspects,init->aspect), init->init_struct, wn );
        } else {
            LOG(CRIT,"Invalid web node initialiser list");
        }
        sw_free(init);
    }

    return wn;
}

web_node* _new_web_node_ooo(wna_flag aspect, ...) {

    va_list args;
    wna_t aspect_list = 0;

    va_start(args,aspect); // init struct for first type is already variadic

    stk_t lo = 0, hi = 0;

    while (aspect != WNA_NONE) {
        while (lo && aspect < ((wna_init_t*)lo->data)->aspect) {
            hi = stack_push(hi,stack_pop(&lo));
        }
        while (hi && aspect > ((wna_init_t*)hi->data)->aspect) {
            lo = stack_push(lo,stack_pop(&hi));
        }
        aspect_list |= aspect;
        wna_init_t* init = sw_malloc(sizeof(wna_init_t*));
        init->aspect = aspect;
        init->init_struct = va_arg(args,void*);
        lo = stack_push(lo,init);
        aspect = va_arg(args,wna_flag);
    } 

    va_end(args);

    stack_flip(&hi, &lo);


    return new_web_node(aspect_list,lo);
}

err_t wn_attach(web_node* parent, web_node* child) {

    if (parent->aspects & WNA_CHILDREN); else {
        LOG(CRIT,"parent unable to have children-> invalid web node attachment");
        return CRIT;
    }

    if (!parent || !child) {
        LOG(WARN,"Invalid web node attachment: parent %p, child %p",parent,child);
        return WARN;
    }

    {
        web_node* root = parent;                    //ID children
        while (!(root->aspects & WNA_ROOT) && root->parent) root = root->parent;
        if (root && (root->aspects & WNA_ROOT)) {
            // Set child trees ids
            stk_t to_process = stack_push(0,child);
            while (to_process) {
                web_node* cur = stack_pop(&to_process);
                cur->id = ++WNA_P_DATA(root,WNA_ROOT)->wnid;
                for (stk_t i = wn_get_children(cur); i; i=i->next)
                    to_process = stack_push(to_process,i->data);
            }
        }
    }

    WNA_P_DATA(parent,WNA_CHILDREN)->subnodes = stack_push(WNA_P_DATA(parent,WNA_CHILDREN)->subnodes, child);
    child->parent = parent;

    return OK;
}


web_node* wn_attach_more(web_node* parent, web_node* children, ...) {
    va_list args;
    va_start(args,children);
    while (children) {
        wn_attach(parent,children);
        children = va_arg(args,web_node*);
    }
    va_end(args);
    
    WNA_P_DATA(parent,WNA_CHILDREN)->subnodes = stk_flip(WNA_P_DATA(parent,WNA_CHILDREN)->subnodes);

    return parent;
}

/*
typedef struct {
    va_list* p_args;
    stk_t* p_sub_h;
} _chtml_cookie_t;
int _chtml_parsefunc(const char* tok, void* cookie) {

    int ret = 0;

    if (!(tok[0] && tok[1] && (*(uint16_t*)(tok) == 0x3a3a) && tok[2])) return ret; // doesn't equal '::' or n undefined

    _chtml_cookie_t* c = cookie;
    long num_sub = strtol(tok+2,0,0);
    while (num_sub--) {
        *c->p_sub_h = stack_push(*c->p_sub_h, va_arg(*c->p_args,web_node*));
    }
}
*/

void wn_apply_tree(web_node* tree, void (*func)(web_node*, void* c),void* cookie) {

    dq_t* parse_q = new_dq(0);
    dq_push_f(parse_q, tree);

    stk_t process_q = 0;

    web_node* cur;
    while (cur = dq_pop_b(parse_q)) {
        stk_t sub = wn_get_children(cur);
        while (sub) {
            dq_push_f(parse_q,sub->data);
            sub = sub->next;
        }
        process_q = stack_push(process_q,cur);
    }

    dlt_dq(parse_q);

    while (cur = stack_pop(&process_q)) {
        func(cur,cookie);
    }
}

typedef struct {
    dummy_handler* s;
    wna_t aspects;
} appa_c_t;

void _apply_aspects( web_node* wn, void* appa_c) {
    for (wna_index i = 0; i<WNA_COUNT; i++) {
        if ((wna_i2f(i) & wn->aspects & ((appa_c_t*)appa_c)->aspects) && wna_aspect_funcs[i]) {
            wna_aspect_funcs[i](((appa_c_t*)appa_c)->s,wn);
        }
    }
}

void wn_apply_all_aspects_to_tree(web_node* rt, dummy_handler* s) {
    wn_apply_tree(rt,_apply_aspects,&(appa_c_t){.aspects = (1 << WNA_COUNT)-1,.s=s});
}

web_node* wn_get_root(web_node* tree) {
    if (tree) while (tree->parent) tree = tree->parent;
    else return 0;
    if (!(tree->aspects&WNA_ROOT)) {
        LOG(WARN,"webnode root does not have root aspect");
        return 0;
    }
    return tree;
}

/*
wna_init_t* init_dummy_aspect(wna_flag a) {
    return wna_init_aspect(a, 0);
}

#define WNA_INIT_CAST(a,struct) wna_init_aspect(a,&(WNA_INIT_STRUCT(a))struct)




web_node* v_pop_wn_sub_html(web_node* wn, va_list* args) {
    _chtml_cookie_t c = {.p_args = args, .p_sub_h = &WNA_P_DATA(wn,WNA_CHILDREN)->subnodes};
    template_token_parse(get_strrs(wn),_chtml_parsefunc,0,&c);
    return wn;
}

web_node* pop_wn_sub_html(web_node* wn, ...) {
    va_list args;
    va_start(args, wn);
    web_node* res = v_pop_wn_sub_html(wn, &args);
    va_end(args);
    return res;
}*/

web_node* wn_html_root() {
    return NEW_WN(WNA_ROOT,0,WNA_HTML(body),
        STRRS_STATIC(REG_HTML_GLOB),
        WNA_CHILDREN, &(WNA_INIT_STRUCT(WNA_CHILDREN)){.subnodes=0}
    );
}

web_node* wn_wrap_html_root(web_node* wn) {
    web_node* root = wn_html_root();
    wn_attach(root,wn);
    return root;
}