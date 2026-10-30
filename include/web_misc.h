#ifndef WEB_MISC_H
#define WEB_MISC_H

#include "include_webc.h"
#include "container.h"


void add_css_class(const char * class_name, const char* style); //out of session

void web_misc_term();

typedef struct {
    const char* id;
    const char* class;
    const char* body_fmt;
    dq_t* sub_html;
} html_element;

void destroy_html_element(void* he);

int deliver_html(dummy_handler* s, html_element* he);

//input html with following templates: {ht} -> sub element : html_element*, {id} -> id : const char*, {st} -> style_class : const char*, {at} : auto tag (class and id if present)
html_element* create_html_element(const char* body_fmt, ...);

#endif //WEB_MISC_H