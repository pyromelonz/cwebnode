#ifndef DOMBASE_H
#define DOMBASE_H

#include "web_node_macro.h"

//nodes should be stk_t of web_nodes

web_node* DOM_vert( stk_t nodes);

web_node* DOM_horz(stk_t nodes);

web_node* _DOM_vert_v(web_node* elements, ...);
web_node* _DOM_horz_v(web_node* elements, ...);

#define DOM_VERT(...) _DOM_vert_v(__VA_ARGS__, 0)
#define DOM_HORZ(...) _DOM_horz_v(__VA_ARGS__, 0)




web_node* DOM_user_header(dummy_handler* s);

/* Reusable authentication components. Form nodes can be embedded in any
 * page; the page nodes add the headings from login.html/register.html. */
web_node* DOM_login_form(void);
web_node* DOM_register_form(void);



/* A CSS_REF node that sets the PARENT HTML element's dimensions.
 * Width and height are CSS values ("320px", "80%", "auto", "20rem", ...).
 * Passing NULL for either one leaves that dimension unchanged.
 * Returns NULL if both dimensions are omitted or a value is unsafe.
 * Attach the returned node to an HTML parent that has WNA_CHILDREN. */
web_node* DOM_box_css(const char* width, const char* height);
web_node* DOM_box_css_px(unsigned width_px, unsigned height_px);

/* Attach a new box CSS node to an existing HTML element (no wrapper).
 * Returns target on success; otherwise NULL without taking ownership. */
web_node* DOM_box_on(web_node* target, const char* width, const char* height);

/* A reusable CSS web node: attach it to an HTML node with WNA_CHILDREN.
 * Colors are red background and black border; variants set dimensions in px.
 * Each invocation returns a NEW node owned by its eventual parent. */
web_node* DOM_red_black_border_css(void);
web_node* DOM_red_black_border_css_custom(unsigned border_px,
                                          unsigned padding_px,
                                          unsigned radius_px);

/* Style an existing HTML node that has WNA_CHILDREN without changing the
 * order of its children. Returns target, or NULL if it cannot be styled. */
web_node* DOM_red_black_border_on(web_node* target);

/* Wrap any existing node, including leaves, in a styled <div>. */
web_node* DOM_red_black_border_box(web_node* content);


#endif //DOMBASE_H
