#include "dombase.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

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
		WL_HTML(a, "href=/index","INDEX"),
		WL_HTML(a, "href=/login","LOGIN"),
		WL_HTML(a, "href=/register","REGISTER"),
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

/* Attribute strings are trusted, static constants (never user input).
 * Each label/input pair is itself a composable web-node tree. */
static web_node* dom_auth_field(const char* title, const char* label_attrs,
                                const char* input_attrs) {
    return NEW_WN(WNA_HTML(div), STRRS_STATIC(REG_HTML_GLOB),
        WNA_SUBNODES(
            NEW_WN(WNA_HTML(label), STRRS_REFSTC(title),
                   STRRS_REFSTC(label_attrs)),
            NEW_WN(WNA_HTML(input), STRRS_REFSTC(""),
                   STRRS_REFSTC(input_attrs))
        ));
}

typedef struct {
    const char* form_attrs;
    const char* username_label_attrs;
    const char* username_input_attrs;
    const char* password_label_attrs;
    const char* password_input_attrs;
    const char* submit_text;
} dom_auth_spec;

static const dom_auth_spec login_spec = {
    .form_attrs = "action=\"/login\" method=\"post\"",
    .username_label_attrs = "for=\"login-username\"",
    .username_input_attrs = "type=\"text\" id=\"login-username\" name=\"username\" autocomplete=\"username\" required",
    .password_label_attrs = "for=\"login-password\"",
    .password_input_attrs = "type=\"password\" id=\"login-password\" name=\"password\" autocomplete=\"current-password\" required",
    .submit_text = "Login"
};

static const dom_auth_spec register_spec = {
    .form_attrs = "action=\"/register\" method=\"post\"",
    .username_label_attrs = "for=\"register-username\"",
    .username_input_attrs = "type=\"text\" id=\"register-username\" name=\"username\" autocomplete=\"username\" required",
    .password_label_attrs = "for=\"register-password\"",
    .password_input_attrs = "type=\"password\" id=\"register-password\" name=\"password\" autocomplete=\"new-password\" required",
    .submit_text = "Register"
};

static web_node* dom_auth_form(const dom_auth_spec* spec) {
    return NEW_WN(WNA_HTML(form), STRRS_STATIC(REG_HTML_GLOB),
        STRRS_REFSTC(spec->form_attrs),
        WNA_SUBNODES(
            dom_auth_field("Username:", spec->username_label_attrs,
                           spec->username_input_attrs),
            dom_auth_field("Password:", spec->password_label_attrs,
                           spec->password_input_attrs),
            NEW_WN(WNA_HTML(button), STRRS_REFSTC(spec->submit_text),
                   STRRS_REFSTC("type=\"submit\""))
        ));
}

web_node* DOM_login_form(void) {
    return dom_auth_form(&login_spec);
}

web_node* DOM_register_form(void) {
    return dom_auth_form(&register_spec);
}




/* CSS dimension tokens may be lengths, percentages, keywords, or functions
 * such as calc(100% - 2rem). Keep declarations and HTML out of user values. */
static int dom_css_dimension_valid(const char* value) {
    if (!value || !*value) return 0;

    unsigned have_letter_or_digit = 0;
    size_t i = 0;
    for (; value[i] && i < 127; ++i) {
        const char ch = value[i];
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9')) {
            have_letter_or_digit = 1;
        } else if (ch != ' ' && ch != '%' && ch != '-' && ch != '+' &&
                   ch != '*' && ch != '/' && ch != '.' && ch != ',' &&
                   ch != '(' && ch != ')' && ch != '_') {
            return 0;
        }
    }
    return !value[i] && have_letter_or_digit;
}

/* A CSS_REF styles its direct parent, not the children of that parent.
 * Use duplicated string storage: style[] is local to this constructor. */
web_node* DOM_box_css(const char* width, const char* height) {
    if ((!width && !height) ||
        (width && !dom_css_dimension_valid(width)) ||
        (height && !dom_css_dimension_valid(height))) return NULL;

    char style[320];
    const int size = snprintf(style, sizeof(style),
        "box-sizing: border-box;%s%s%s%s%s%s",
        width ? " width: " : "", width ? width : "", width ? ";" : "",
        height ? " height: " : "", height ? height : "", height ? ";" : "");
    if (size < 0 || (size_t)size >= sizeof(style)) return NULL;

    return NEW_WN(WNA_CSS_REF, 0, STRRS_REFDUP(style));
}

web_node* DOM_box_css_px(unsigned width_px, unsigned height_px) {
    char width[24], height[24];
    snprintf(width, sizeof(width), "%upx", width_px);
    snprintf(height, sizeof(height), "%upx", height_px);
    return DOM_box_css(width, height);
}

web_node* DOM_box_on(web_node* target, const char* width, const char* height) {
    if (!target || !(target->aspects & WNA_HTML_BODY) ||
        !(target->aspects & WNA_CHILDREN)) return NULL;

    web_node* style = DOM_box_css(width, height);
    if (!style) return NULL;
    if (wn_attach(target, style) != OK) {
        delete_web_node(style);
        return NULL;
    }
    return target;
}

/* A CSS_REF node styles its parent HTML element through the generated ID.
 * Unlike global CSS, each instance can be attached wherever it is needed. */
web_node* DOM_red_black_border_css_custom(unsigned border_px,
                                          unsigned padding_px,
                                          unsigned radius_px) {
    char style[192];
    int count = snprintf(style, sizeof(style),
        "background-color: red; border: %upx black; "
        "padding: %upx; border-radius: %upx; box-sizing: border-box;",
        border_px, padding_px, radius_px);
    if (count < 0 || (size_t)count >= sizeof(style)) return NULL;

    /* STRRS_REFDUP is important: style[] is only alive during this call. */
    return NEW_WN(WNA_CSS_REF, 0, STRRS_REFDUP(style));
}

web_node* DOM_red_black_border_css(void) {
    return DOM_red_black_border_css_custom(10, 5, 0);
}

web_node* DOM_red_black_border_on(web_node* target) {
    if (!target || !(target->aspects & WNA_HTML_BODY) ||
        !(target->aspects & WNA_CHILDREN)) return NULL;

    web_node* style = DOM_red_black_border_css();
    if (!style) return NULL;
    if (wn_attach(target, style) != OK) {
        delete_web_node(style);
        return NULL;
    }
    return target;
}

web_node* DOM_red_black_border_box(web_node* content) {
    if (!content) return NULL;
    return NEW_WN(WNA_HTML(div), STRRS_STATIC(REG_HTML_GLOB),
        WNA_SUBNODES(content, DOM_red_black_border_css()));
}
