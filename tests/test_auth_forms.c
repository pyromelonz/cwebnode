#define _GNU_SOURCE
#include "dombase.h"
#include "wn_component_registry.h"
#include "log.h"
#include "user_data.h"
#include "user_session.h"
#include "mem_dbg.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* (*_sw_malloc)(size_t) = malloc;
void* (*_sw_calloc)(size_t,size_t) = calloc;
char* (*_sw_strdup)(const char*) = strdup;
void (*_sw_free)(void*) = free;
void* (*_sw_realloc)(void*, size_t) = realloc;

const char* static_comp_reg[_STATIC_WWW_COMPONENTS_COUNT] = {
    [REG_INVALID] = "invalid", [REG_HTML_GLOB]="{::}",
    [REG_DISPLAY_INLINE_BLOCK_CHILDREN] = "{c>}display: inline-block",
    [REG_DISPLAY_INLINE_BLOCK] = "display: inline-block"
};
void cr_block_rehash(void) {}
void cr_unblock_rehash(void) {}
const char* comp_reg_ambi_get(reg_ambindex unused) { (void)unused; return ""; }
const char* comp_reg_get(const char* k) { (void)k; return NULL; }
void* comp_reg_data(size_t i) { (void)i; return NULL; }
entry_t* comp_reg_get_p(const char* k) { (void)k; return NULL; }
entry_t* comp_reg_set(const char* k, void* data, enum COMP_REG_STORE_FLAGS flags) { (void)k; (void)data; (void)flags; return NULL; }

uid_t session_get_user(void* s) { (void)s; return 0; }
const char* user_get_pnym(uid_t u) { (void)u; return "Guest"; }
void log_msg(err_t level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr,"LOG %d: ", level);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr,"\n");
    va_end(ap);
}

void printHTML(dummy_handler* s, const char* fmt, ...) {
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy,args);
    int amount = vsnprintf(NULL,0,fmt,copy);
    va_end(copy);
    assert(amount>=0);
    size_t needed=s->length + (size_t)amount + 1;
    if (needed>s->capacity) {
        s->capacity=needed*2;
        s->output=realloc(s->output,s->capacity);
        assert(s->output);
    }
    vsnprintf(s->output+s->length,s->capacity-s->length,fmt,args);
    s->length+=(size_t)amount;
    va_end(args);
}

static void check_page(int register_page) {
    dummy_handler s = {0};
    web_node* root = wn_wrap_html_root(register_page ? DOM_register_page() : DOM_login_page());
    assert(wn_deliver(&s,root)==OK);
    assert(s.output);
    if (register_page) {
        assert(strstr(s.output,"<form action=\"/register\" method=\"post\">")!=NULL);
        assert(strstr(s.output,"<h1>New User Registration</h1>")!=NULL);
        assert(strstr(s.output,"<button type=\"submit\">Register</button>")!=NULL);
        assert(strstr(s.output,"for=\"register-username\"")!=NULL);
        assert(strstr(s.output,"id=\"register-password\"")!=NULL);
    } else {
        assert(strstr(s.output,"<form action=\"/login\" method=\"post\">")!=NULL);
        assert(strstr(s.output,"<h1>Login User</h1>")!=NULL);
        assert(strstr(s.output,"<button type=\"submit\">Login</button>")!=NULL);
        assert(strstr(s.output,"for=\"login-username\"")!=NULL);
        assert(strstr(s.output,"id=\"login-password\"")!=NULL);
    }
    assert(strstr(s.output,"name=\"username\"")!=NULL);
    assert(strstr(s.output,"name=\"password\"")!=NULL);
    assert(strstr(s.output,"type=\"password\"")!=NULL);
    assert(strstr(s.output," required>")!=NULL);
    assert(strstr(s.output,"</input>")==NULL);
    delete_web_node(root);
    free(s.output);
}

static void check_route(int registering, const char* post_data);
static unsigned login_calls, register_calls;
int main(void) {
    check_page(0);
    check_page(1);
    check_route(0, NULL);
    check_route(1, NULL);
    check_route(0, "username=alice&password=secret");
    check_route(1, "username=alice&password=secret");
    assert(login_calls == 1 && register_calls == 1);
    fprintf(stderr,"Authentication DOM and route smoke tests passed.\n");
}

/* Route-level checks: actual GET and POST handlers, using fake session APIs. */
extern WS_FILE_TYPES _func_login(dummy_handler*);
extern WS_FILE_TYPES _func_register(dummy_handler*);
size_t getPostSize(dummy_handler* s) { return s->post_data ? strlen(s->post_data) : 0; }
const char* getPostData(dummy_handler* s) { return s->post_data; }
uid_t session_login_user(void* s, const char* username, const char* password) {
    (void)s;
    assert(strcmp(username,"alice")==0);
    assert(strcmp(password,"secret")==0);
    login_calls++;
    return 1;
}
uid_t session_register_user(void* s, const char* username, const char* password) {
    (void)s;
    assert(strcmp(username,"alice")==0);
    assert(strcmp(password,"secret")==0);
    register_calls++;
    return 1;
}
void session_logout(void* s) { (void)s; }
uid_t session_remove_user(void* s, const char* username) { (void)s; (void)username; return 0; }
WS_FILE_TYPES redirect(dummy_handler* s, const char* url) {
    printHTML(s,"REDIRECT %s",url);
    return WS_FILE_TYPE_CUSTOM;
}

static void check_route(int registering, const char* post_data) {
    dummy_handler s = {0};
    s.post_data = post_data;
    WS_FILE_TYPES result = registering ? _func_register(&s) : _func_login(&s);
    if (post_data) {
        assert(result == WS_FILE_TYPE_CUSTOM);
        assert(strstr(s.output,"REDIRECT /index")!=NULL);
    } else {
        assert(result == WS_FILE_TYPE_HTML);
        assert(strstr(s.output, registering ? "action=\"/register\"" : "action=\"/login\"")!=NULL);
        assert(strstr(s.output, registering ? "<button type=\"submit\">Register</button>" : "<button type=\"submit\">Login</button>")!=NULL);
    }
    free(s.output);
}
