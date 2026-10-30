#ifndef SITES_MACRO_H
#define SITES_MACRO_H

#include "sites.h"

typedef struct {
    const char* url;
    USER_GROUPS access_groups;
    WS_FILE_TYPES (*func)(dummy_handler* s);
} web_resource;

//these are added during sites_init:

#define WEB_RESOURCE_LIST \
    X(register) \
    X(remove) \
    X(login) \
    X(logout) \
    X(index) \
    X(test_now) \
    X(upload_file) \
    X(promote_user) \
    X(shutdown_dev) \

#define WR_DAT_NAME(x) _wr_dat##x

#define X(a) extern web_resource WR_DAT_NAME(a);
    WEB_RESOURCE_LIST
#undef X

#define DEFINE_WEB_RESOURCE(url, groups) \
    WS_FILE_TYPES _func_##url(dummy_handler*); \
    web_resource WR_DAT_NAME(url) = {#url, groups, _func_##url}; \
    WS_FILE_TYPES _func_##url(dummy_handler* s)

/*  Macro to define and simultaneously register url functions to the engine

#define MAKE_WEB_RESOURCE(url, groups)                                          \
    WS_FILE_TYPES _func_##url(dummy_handler*);             \
    err_t __attribute__((constructor)) _reg_func_##url() {                     \
        return web_resource_add( _func_##url, #url, 0, groups );                \
    }                                                                          \
    WS_FILE_TYPES _func_##url(dummy_handler* s)

 Just do MAKE_WEB_RESOURCE("desired_url"){function body} */

#endif //SITES_MACRO_H
