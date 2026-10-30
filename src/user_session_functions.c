#include "sites_macro.h"

#include "misc.h"
#include "user_session.h"
#include "user_data.h"
#include "config.h"

#include <string.h>

#ifdef SESSION_TESTING
        #include "user_data.h"
        #include "user_auth.h"
#endif

//after macro expansion register is never parsed as a singular token - no problems
DEFINE_WEB_RESOURCE(register, ALL_USER_GROUPS ) {

    unsigned post_size = getPostSize(s);

    uid_t user;

    if (post_size) {
        LOG(DBG,"post data: %s",getPostData(s));
        char *un = 0, *pw = 0;
        if (2 > read_post_data(getPostData(s), "username password",&un,&pw)) user = 0;
        else {
            user = session_register_user(s,un,pw);
        }
        if (un) sw_free(un);
        if (pw) sw_free(pw);
    }

    #ifdef SESSION_TESTING
     #include "user_data.h"
        printHTML(s, user && user_get_pnym(user) && user == session_get_user(s) ? "registration success\n" : "registration failure\n");
        return WS_FILE_TYPE_HTML;
    #endif

    if (user) {
        return redirect(s,"/index");
    } else {
        printHTML(s,"registration failure\n");
        return WS_FILE_TYPE_HTML;
    }
}

DEFINE_WEB_RESOURCE(login, GUEST | USER) {
    uid_t user = 0;

    if (getPostSize(s)) {  //form filled in
        
        LOG(DBG,"post data: %s",getPostData(s));
        char* un= 0, *pw = 0;
        if ( 2 > read_post_data(getPostData(s), "username password",&un,&pw)) {
            user = 0;
        } else {
            user = session_login_user(s,un, pw);
        }
        if (un) sw_free(un);
        if (pw) sw_free(pw);
    }

    #ifdef SESSION_TESTING
        printHTML(s, user == session_get_user(s) && user ? "login success\n" : "login failure\n");
        return WS_FILE_TYPE_HTML;
    #endif

    if (user) {
        return redirect(s,"/index");
    } else {
        printHTML(s,"login failure\n");
        return WS_FILE_TYPE_HTML;
    }
}

DEFINE_WEB_RESOURCE(logout,USER ) {

    if (session_get_user(s)) {
        session_logout(s); //I dont see a world where this could fail
    }

    #ifdef SESSION_TESTING
        else {
            printHTML(s, "logout failure\n");
            return WS_FILE_TYPE_HTML;
        }
        printHTML(s, session_get_user(s) ? "logout failure\n" : "logout success\n");
        return WS_FILE_TYPE_HTML;
    #endif
    
    return redirect(s, "/index");
}

DEFINE_WEB_RESOURCE(remove,ADMIN) {
    unsigned post_size = getPostSize(s);

    uid_t user;

        char *un = 0;
    if (post_size) {
        LOG(DBG,"post data: %s",getPostData(s));
        if (1 > read_post_data(getPostData(s), "username",&un)) user = 0;
        else {
            user = session_remove_user(s,un);
        }
    
    #ifdef SESSION_TESTING
        LOG(DBG,"remaining user: %u, user_data: %p, user_uid: %u",user,user_data_get(user),user_get_uid(un));
        printHTML(s, user && !user_data_get(user) && !user_get_uid(un) ? "removal success\n" : "removal failure\n");
        return WS_FILE_TYPE_HTML;
    #endif

    }

    if (un) sw_free(un);

    if (user) {
        return redirect(s,"/index");
    } else {
        printHTML(s,"removal failure\n");
        return WS_FILE_TYPE_HTML;
    }
}