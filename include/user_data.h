#ifndef USER_DATA_H
#define USER_DATA_H

#include "types.h"

typedef struct {
    USER_GROUPS groups;
} user_misc_data;

void user_init(), user_term();

void user_data_add(uid_t uid, const char* username);
void user_data_remove(uid_t uid);

//add session to users authorised sessions and username as session variable
void user_authorize_session(void* s, uid_t uid),
    user_unauthorize_session(void* s, uid_t uid);

char user_is_session_authorized(void* s, uid_t uid);

user_misc_data* user_data_get(uid_t u);

const char* user_get_pnym(uid_t uid);

#ifdef DEBUG
void user_print_authorised_sessions(uid_t uid);
#endif

#endif //USER_DATA_H
