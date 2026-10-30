#include "user_data.h"
#include "user_auth.h"
#include "misc.h"
#include "sites.h"
#include <string.h>
#include "webserver_api_functions.h"
#include "config.h"
#include "tree_misc.h"
#include <pthread.h>

pthread_mutex_t ud_mutex = PTHREAD_MUTEX_INITIALIZER;

struct guid_list_t {
    const char* val;
    struct guid_list_t* next;
};

typedef struct guid_list_t guid_list_t;

typedef struct user_data_t {
    uid_t uid;
    guid_list_t* authorised_sessions;
    char username [USERNAME_MAX_SIZE +1];
    user_misc_data misc;
} user_data_t;


rbtree* user_data;
static int _user_data_cmp(const void* _a, const void* _b) {
    const user_data_t *a = _a, *b = _b;
    return (a->uid > b->uid) - (a->uid < b->uid);
}

static void clear_guid_list(guid_list_t* gl) {
    while (gl) {
        guid_list_t* tmp = gl;
        sw_free((void*)gl->val);
        gl = gl->next;
        sw_free(tmp);
    }
}

int apply_clear_guid_list(void* data, void* cookie) {
    clear_guid_list(((user_data_t*) data)->authorised_sessions);
    ((user_data_t*) data)->authorised_sessions = 0;
    return 0;
}

static void _user_free(void* _u) {
    clear_guid_list (((user_data_t*) _u)->authorised_sessions);
    sw_free(_u);
}

void user_init() {
	user_data = rb_create(_user_data_cmp,_user_free);

    #ifdef LOAD_USERS
    tree_load(USER_DATA_FILE, user_data, sizeof(user_data_t));
    #else
    user_data_add(user_register(ROOT_USERNAME, ROOT_PASSWORD), ROOT_USERNAME);
    user_data_get(user_get_uid(ROOT_USERNAME))->groups |= ALL_USER_GROUPS;
    #endif
}

void user_term() {
    #ifdef SAVE_USERS
    RB_APPLY(user_data, apply_clear_guid_list, 0, INORDER);
    tree_save(USER_DATA_FILE, user_data, sizeof(user_data_t));
    #endif
	rb_destroy(user_data);
}

static user_data_t* get_user(uid_t uid) {
    user_data_t* ud = sw_malloc(sizeof(user_data_t));
    ud->uid = uid;

    rbnode* fnd = rb_find(user_data, ud);
    sw_free (ud);

    return fnd ? (fnd->data) : 0;
}

user_misc_data * user_data_get(uid_t u) {
    user_data_t* udata = get_user(u);
    return udata ? &udata->misc : 0;
}


void user_data_add_g(uid_t uid, const char* username, USER_GROUPS groups) {
    pthread_mutex_lock(&ud_mutex);
    if (get_user(uid)) {
        LOG(WARN,"Tried to add user %s with pre-existing uid %u to database", username, uid);
    } else {
        user_data_t* ud = sw_malloc(sizeof(user_data_t));
        ud->uid = uid;
        strncpy(ud->username,username,USERNAME_MAX_SIZE);
        ud->username[USERNAME_MAX_SIZE] = 0;
        ud->authorised_sessions = 0;
        ud->misc.groups = groups;
        rb_insert(user_data,ud);
    }
    pthread_mutex_unlock(&ud_mutex);
}

void user_data_add(uid_t uid, const char* username) {
    user_data_add_g(uid,username,USER);
}

void user_data_remove(uid_t uid) {
    pthread_mutex_lock(&ud_mutex);
    user_data_t* ud = get_user(uid);
    if (!ud) {
        LOG(WARN,"Tried to remove non-existent user with uid %u from database", uid);
        pthread_mutex_unlock(&ud_mutex);
        return;
    }
    rbnode* fnd = rb_find(user_data, ud);
    if (fnd) {
        rb_delete(user_data, fnd, 0);
    } else {
        LOG(WARN,"couldn't find user with uid %u in database for removal", uid);
    }
    pthread_mutex_unlock(&ud_mutex);
    //user free function will be called by rb_delete
}

//add session to users authorised sessions and username as session variable
void user_authorize_session(void* s, uid_t uid) {
    if (user_is_session_authorized(s,uid)) {
        LOG(MSG,"Session %s already authorised for uid %u",getSessionGUID(s),uid);
        return;
    }
    user_data_t* u = get_user(uid);
    if (!u) {
        LOG(DBG, "No user with uid %u found to authorise session for",uid);
        return;
    }
    setSessionVar(s,STORE_NORMAL,"username",u->username);
    setSessionVar(s,STORE_SSL,"username",u->username);

    LOG(DBG,"set session variables for user %s",u->username);

    guid_list_t* sessions = sw_malloc(sizeof(guid_list_t));
    sessions->val = sw_strdup(getSessionGUID(s));
    sessions->next = u->authorised_sessions;
    u->authorised_sessions = sessions;
    setUserRegisterStatus(s,SESSION_REGISTERED);

    LOG(MSG,"Authorised Session %s for uid %u",getSessionGUID(s),uid);
}

void user_unauthorize_session(void* s, uid_t uid) {

    setSessionVar(s,STORE_NORMAL,"username",NULL);
    setSessionVar(s,STORE_SSL,"username",NULL);
    setUserRegisterStatus(s,NOT_REGISTERED);


    LOG(MSG,"Unauthorised Session %s for uid %u",getSessionGUID(s),uid);

    guid_list_t **list = &get_user(uid)->authorised_sessions;

    while (*list) {
        guid_list_t *node = *list;
        if (!strcmp(node->val, getSessionGUID(s))) {
            *list = node->next;            
            sw_free((void*)node->val);
            sw_free(node);
        } else {
            list = &node->next;
        }
    }

}

char user_is_session_authorized(void* s, uid_t uid) {
    if (!uid || !get_user(uid)) return 0;
    guid_list_t* sl = get_user(uid)->authorised_sessions;
    while (sl) {
        if (!strcmp(sl->val,getSessionGUID(s))) {
            return 1;
        }
        sl = sl->next;
    }
    return 0;
}

const char* user_get_pnym(uid_t uid) {
    user_data_t* u = get_user(uid);
    return u ? u->username : "Guest";
}

#ifdef DEBUG
void user_print_authorised_sessions(uid_t uid) {
    if (!uid) return;
    guid_list_t* l = get_user(uid)->authorised_sessions;
    while (l) {
        LOG(DBG,"SESSION %s is authorised for uid %u",l->val,uid);
        l = l->next;
    }
}
#endif