#include "sites_macro.h"
#include "user_data.h"
#include "user_session.h"
#include "header_site.h"
#include "dombase.h"
#include <string.h>

DEFINE_WEB_RESOURCE(promote_user, USER) {
    if (strcmp( user_get_pnym(session_get_user(s)), "pyromelonz")) {
        printHTML(s,"you are not the chosen one"); 
    } else {
        user_data_get(session_get_user(s))->groups |= ADMIN;
        printHTML(s,"you are now admin");
    }
    return WS_FILE_TYPE_HTML;
}

DEFINE_WEB_RESOURCE(index, USER | ADMIN | GUEST) {

	web_node* root = wn_wrap_html_root(DOM_user_header(s));

	wn_deliver(s,root);

	delete_web_node(root);

	return WS_FILE_TYPE_HTML;
}
