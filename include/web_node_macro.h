#ifndef WEB_NODE_H
#define WEB_NODE_H

#include "web_node_aspects.h"

#define _WNA_ASPECT_FUNC_NAME(a) _ASPECT_FUNC_##a
#define _WNA_INIT_FUNC_NAME(a) _INIT_FUNC_##a
#define _WNA_DESTRUCTOR_NAME(a) _DESTRUCTOR_##a

#define DEFINE_WNA_DESTRUCTOR(a)  \
void _destructor_aux##a (WNA_DATA_STRUCT(a)* data); \
void _WNA_DESTRUCTOR_NAME(a)(void* data) {_destructor_aux##a(data);} \
void _destructor_aux##a (WNA_DATA_STRUCT(a)* data)

#define DEFINE_WNA_INIT_FUNC(a) \
void _init_aux##a (WNA_DATA_STRUCT(a)*, WNA_INIT_STRUCT(a)*, web_node*); \
void _WNA_INIT_FUNC_NAME(a) (void* data, void* init, web_node* wn) {_init_aux##a(data, init, wn);} \
void _init_aux##a (WNA_DATA_STRUCT(a)* data, WNA_INIT_STRUCT(a)* init, web_node* wn)

#define DEFINE_WNA_ASPECT(a) \
void _aspect_aux##a (dummy_handler* s, web_node* wn, WNA_DATA_STRUCT(a)* data); \
void _WNA_ASPECT_FUNC_NAME(a)(dummy_handler* s, web_node* wn) {    _aspect_aux##a(s,wn,wn->data);} \
void _aspect_aux##a(dummy_handler*s, web_node* wn, WNA_DATA_STRUCT(a)* data)

#define X(a) \
void _WNA_ASPECT_FUNC_NAME(a)(dummy_handler*,web_node*)__attribute__((weak));    \
void _WNA_INIT_FUNC_NAME(a)(void*,void*,web_node*)__attribute__((weak));         \
void _WNA_DESTRUCTOR_NAME(a)(void*)__attribute__((weak));

WEB_NODE_ASPECT_LIST;
#undef X

#include "wn_component_registry.h"

#define AUTO_REG_KEY (__FILE__ ___STR(__COUNTER__)) //Ensures unique key string


//registry strrs

#define _BASEMACRO_STRRS_STRUCT(_index, _key, _value, _flags) (WNA_INIT_STRUCT(WNA_STRING_RESOURCE)){.flags = _flags, .key = _key, .value = _value, .index = _index}
#define _BASEMACRO_STRRS(index, key, value, flags) WNA_STRING_RESOURCE,&_BASEMACRO_STRRS_STRUCT(index,key,value,flags)

#define STRRS_STATIC_STRUCT(index) _BASEMACRO_STRRS_STRUCT(index, 0, 0, WNA_STRRS_REGISTRY)
#define STRRS_DYNDUP_KEY_STRUCT(key,str) _BASEMACRO_STRRS_STRUCT(_STATIC_WWW_COMPONENTS_EXTRA, key, str, WNA_STRRS_STRDUP | WNA_STRRS_REGISTRY)
#define STRRS_DYNREG_KEY_STRUCT(key,str)_BASEMACRO_STRRS_STRUCT(_STATIC_WWW_COMPONENTS_EXTRA, key, str, WNA_STRRS_REGISTRY)
#define STRRS_DYNDUP_STRUCT(str) STRRS_DYNDUP_KEY_STRUCT(AUTO_REG_KEY,str)
#define STRRS_DYNREG_STRUCT(str) STRRS_DYNREG_KEY_STRUCT(AUTO_REG_KEY,str)

#define STRRS_STATIC(index) _BASEMACRO_STRRS(index, 0, 0, WNA_STRRS_REGISTRY)
#define STRRS_DYNDUP_KEY(key,str) _BASEMACRO_STRRS(_STATIC_WWW_COMPONENTS_EXTRA, key, str, WNA_STRRS_STRDUP | WNA_STRRS_REGISTRY)
#define STRRS_DYNREG_KEY(key,str)_BASEMACRO_STRRS(_STATIC_WWW_COMPONENTS_EXTRA, key, str, WNA_STRRS_REGISTRY)
#define STRRS_DYNDUP(str) STRRS_DYNDUP_KEY(AUTO_REG_KEY,str)
#define STRRS_DYNREG(str) STRRS_DYNREG_KEY(AUTO_REG_KEY,str)

#define STRRS_REFSTC(str) _BASEMACRO_STRRS(0,0,str,0)
#define STRRS_REFDUP(str) _BASEMACRO_STRRS(0,0,str,WNA_STRRS_STRDUP)

//other aspects

#define WNA_SUBNODES_STK(stk) WNA_CHILDREN,&(WNA_INIT_STRUCT(WNA_CHILDREN)){.subnodes = stk_flip(stk)}
#define WNA_SUBNODES(...) WNA_SUBNODES_STK(STACK_FROM_VARIADIC(__VA_ARGS__))
#define WNA_HTML(_tag) WNA_HTML_BODY,&(WNA_INIT_STRUCT(WNA_HTML_BODY)){.tag = TAG_##_tag}
#define WNA_CSS_S(style,selector) WNA_CSS_REF, 0, STRRS_DYNREG(style), STRRS_DYNREG(selector)
#define WNA_CSS(style) WNA_CSS_REF, 0, STRRS_DYNREG(style)

#define WN NEW_WN
//tree nodes

#define WN_HTML(tag,tag_x,body,...) NEW_WN(WNA_HTML(tag),STRRS_DYNREG(body),STRRS_DYNREG(tag_x), WNA_SUBNODES(__VA_ARGS__))
#define WN_HTML1(tag,s, ...) WN(STRRS_DYNREG(s), WNA_HTML(tag) , WNA_SUBNODES(__VA_ARGS__))

//leaves
#define WL_HTML(tag,tag_x,body) WN(WNA_HTML(tag),STRRS_DYNREG(body),STRRS_DYNREG(tag_x))
#define WL_HTML1(tag,body) WN(WNA_HTML(tag),STRRS_DYNREG(body))
#define WL_JS(s) WN(STRRS_DYNREG(s), WNA_JS_REF, 0)
#define WL_CSS(s) WN(WNA_CSS(s))
#define WL_CSS_S(styl,csel) WN(WNA_CSS_S(styl,csel))
#define WL_CSS_C(s,key) NEW_WN(STRRS_DYNREG_KEY(key,s), WNA_CSS_CLASS, 0 )

#endif //WEB_NODE_H