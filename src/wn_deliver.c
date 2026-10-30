#include "web_node_macro.h"
#include "rb.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>
#include "wn_component_token.h"

typedef struct {dummy_handler* s; wnid_t id;} ref_tokc_t;

wn_token_t js_tokf(const char* tok, void* cookie) {
    ref_tokc_t* c = cookie;
    switch (tok16(tok)) {
        case TK16_ID: printHTML(c->s,"\"wn%u\"",c->id); return WN_TOKEN_ID;
        case TK16_JS_DOM: printHTML(c->s, "document.getElementById(\"wn%u\")", c->id); return WN_TOKEN_INDEX_JS_DOM;
        default: LOG(WARN,"Invalid template found in css_tokf in wn_deliver");
    }
    return 0;
}

wn_token_t ref_restf(const char* s, void* cookie) {
    ref_tokc_t* c = cookie;
    printHTML(c->s,"%s",s);
    return 0;
}

int wn_deliver_js(dummy_handler* s, rs_delivery_t* idrs) {
    template_token_parse_v(get_strrs(idrs->rs,0),js_tokf,ref_restf,&(ref_tokc_t){.id=idrs->id,.s=s},"{{","}}");
}

int wn_deliver_css_class(dummy_handler* s, reg_ambindex rai) {
    printHTML(s,".cl%u {%s}\n",rai,comp_reg_ambi_get(rai));
}

int wn_deliver_css_ref(dummy_handler* s, rs_delivery_t* idrs) {
    web_node* rs = idrs->rs;
    const char* sel = (rs->n_str_params > 1) ? get_strrs(rs,1) : "",
                * style = get_strrs(rs,0);
    printHTML(s,"#wn%u %s {%s}\n",idrs->id, sel, style);
}

typedef struct {
    dummy_handler*s; stk_t html_subs; web_node* wn;
}_dhtfc;

int wn_deliver_html(dummy_handler*,web_node*);

int wn_html_print_classes(web_node* wn, dummy_handler* s) {
    char sp = 0;
    for (stk_t sub = wn_get_children(wn); sub; sub= sub->next) {
        if (((web_node*)sub->data)->aspects & WNA_CSS_CLASS) {
            printHTML(s,"%scl%u", sp ? " " : "", WNA_P_DATA(sub->data,WNA_CSS_CLASS)->clid); sp = 1;
        }
    }
}

wn_token_t deliver_html_token_func(const char* token, void* cookie) {
    _dhtfc* c = cookie;
    
    if (!token[0] || !token[1]) {
        LOG(WARN,"Short or empty token found in html token function");
        return WARN;
    }

    long n_sub_html = -1;

    switch (tok16(token)) {
        case TK16_ID: /*id*/ printHTML(c->s,"wn%u",c->wn->id); break;
        case TK16_CL: /*cl*/ wn_html_print_classes(c->wn,c->s); break;
        case TK16_GLOB:
            if (token[2]) {
                n_sub_html = strtol(token+2,0,0);
                //LOG(DBG,"number of subelements required: %d from strol of %s + 2 pointer", n_sub_html, token);
            }
            while (n_sub_html && c->html_subs ) {
                if (((web_node*)c->html_subs->data)->aspects&WNA_HTML_BODY) {
                    wn_deliver_html(c->s,c->html_subs->data);
                    n_sub_html--;
                }
                c->html_subs = c->html_subs->next;
            }
            if (n_sub_html > 0) LOG(WARN,"Couldn't deliver all HTML elements required by html :: token");
            break;
        default:
            LOG(WARN,"Invalid token found in html token function");
    }
    return 0;
}

const char const * const HTML_tag_strings[] = {
    #define X(t) #t,
    HTML_TAGLIST
    #undef X
};

wn_token_t deliver_html_rest_func(const char* str, void* cookie) {
    printHTML(((_dhtfc*)cookie)->s,str);
}

int wn_deliver_html(dummy_handler* s, web_node* wn) {

    if (wn->n_str_params); else return -1;

    const WNA_DATA_STRUCT(WNA_HTML_BODY)* data = WNA_P_DATA(wn,WNA_HTML_BODY);

    if (data->tag == TAG_plain) {
        printHTML(s,"%s", get_strrs(wn,0));
        return 0;
    }

    printHTML(s,"<%s",HTML_tag_strings[data->tag]);

    if (data->flags & WNA_HTML_CLASS_TARGET) {
        printHTML(s," class=\"");
        wn_html_print_classes(wn,s);
        printHTML(s,"\"");
    }

    if (data->flags & WNA_HTML_ID_TARGET) printHTML(s, " id=\"wn%u\"", wn->id);
    if (wn->n_str_params>1) printHTML(s, " %s", get_strrs(wn,1));
    printHTML(s,">");

    const char* str = get_strrs(wn,0);

    template_token_parse(str,deliver_html_token_func,deliver_html_rest_func,&(_dhtfc)
        {.wn=wn, .html_subs = (wn->aspects & WNA_CHILDREN) ? WNA_P_DATA(wn,WNA_CHILDREN)->subnodes : 0, .s = s});
    printHTML(s,"</%s>",HTML_tag_strings[data->tag]);
    return 0;
}

static const char head_tmp[] = "<head>"
"  <meta charset=\"UTF-8\" />"
"  <title>Hello, world!</title>"
"  <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\" />"
"  <meta name=\"description\" content=\"\" />"
"  <link rel=\"icon\" href=\"favicon.png\">"
"</head>";


static void css_deliver_helper(size_t i, void* cookie) {
    wn_deliver_css_class(cookie,i);
}

err_t wn_deliver(dummy_handler* s, web_node* wn) {

    cr_block_rehash();

    wn_apply_all_aspects_to_tree(wn,s);

    WNA_DATA_STRUCT(WNA_ROOT)* rtd = WNA_P_DATA(wn,WNA_ROOT);

    printHTML(s,"<!DOCTYPE html>\n ");
        
    printHTML(s,"<style>\n");

    while (rtd->css_refs) {
        rs_delivery_t* idrs = stack_pop(&rtd->css_refs);
        wn_deliver_css_ref(s,idrs);
        sw_free(idrs);
    }

    bs_foreach(rtd->css_classes_bs, css_deliver_helper,s);

    cr_unblock_rehash();

    printHTML(s,"</style>\n");
    printHTML(s,"<html lang=\"en\">\n%s\n",head_tmp);

    if (wn->aspects&WNA_HTML_BODY) {
        wn_deliver_html(s,wn);
    }

    printHTML(s,"<script>\n");

    while (rtd->js_refs) {
        rs_delivery_t* idrs =stack_pop(&rtd->js_refs);
        wn_deliver_js(s,idrs);
        sw_free(idrs);
    }

    printHTML(s,"</script>\n");

    return OK;

}
