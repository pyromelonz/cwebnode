#include "dombase.h"
#include <stdio.h>
#include <stdarg.h>

web_node* DOM_vert( stk_t nodes) {
    for (stk_t i = nodes; i; i=i->next) {
        i->data = NEW_WN(STRRS_STATIC(REG_HTML_GLOB),WNA_HTML(li),
		WNA_SUBNODES(
			i->data
		));
    }
    web_node* container = NEW_WN(WNA_HTML(ul), 
		WNA_SUBNODES_STK(nodes), STRRS_STATIC(REG_HTML_GLOB)
	);
	return container;
}

web_node* DOM_horz(stk_t nodes) {
	web_node* ret = DOM_vert(nodes);
	wn_attach(ret,NEW_WN(WNA_CSS_REF,0,STRRS_STATIC(REG_DISPLAY_INLINE_BLOCK_CHILDREN)));
	return ret;
}

web_node* _DOM_vert_v(web_node* elements, ...) {
	stk_t i = 0;
	va_list args;
	va_start(args,elements);
	while (elements) {
		i = stack_push(i,elements);
		elements = va_arg(args,web_node*);
	}
	va_end(args);
	return DOM_vert(i);
}

web_node* _DOM_horz_v(web_node* elements, ...) {
	stk_t i = 0;
	va_list args;
	va_start(args,elements);
	while (elements) {
		i = stack_push(i,elements);
		elements = va_arg(args,web_node*);
	}
	va_end(args);
	return DOM_horz(i);
}

web_node* _plain_container() {
	return NEW_WN(WNA_HTML(div), STRRS_STATIC(REG_HTML_GLOB), WNA_CHILDREN, 0);
}

web_node* DOM_flex_row() {
	return WN_ADD(_plain_container(),WL_CSS_C("flex-direction: row","flex_row_class"));
}

#include "user_session.h"
#include "user_data.h"

web_node* DOM_user_greeting(dummy_handler* s) {
	web_node* cont = WN_ADD(_plain_container(), WL_HTML1(plain,"Welcome, "), WN(WNA_HTML(plain),STRRS_REFDUP(user_get_pnym(session_get_user(s)))));
	return cont;
}


web_node* DOM_user_header(dummy_handler* s) {
	web_node* container = WN_ADD(DOM_flex_row(),
		WL_CSS("background-color : blue; height : 100px; display : flex"),
		WL_CSS_S("color : red; padding : 10px; flex : 1","> *"),
		WL_CSS_S("display: block; background-color: blue; color: red;padding: 10px","> a"),
		DOM_user_greeting(s),
		WL_HTML(a, "href=index.html","INDEX"),
		WL_HTML(a, "href=login.html","LOGIN"),
		WL_HTML(a, "href=register.html","REGISTER"),
		WN_HTML1(div,"{::}",
			WL_HTML1(plain,"Dotted lines, woah!"),
			WL_JS(
  				"const el = {{dE}};"
  				"const texts = [\"Loading...\", \"Please wait\"];"
  				"let index = 0;"
  				"setInterval(() => {"
  				"  index = 1 - index;"
  				"  el.textContent = texts[index];"
  				"}, 1000);"),
			WL_CSS_C("border-style : dotted; padding : 3px","dottedborderclass"))
	);
	return container;
}

