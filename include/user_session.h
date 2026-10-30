#ifndef USER_SESSION_H
#define USER_SESSION_H

#include "types.h"

uid_t session_register_user(void* s, const char* username, const char* password);

uid_t 	session_get_user(void* s),
		session_login_user(void* s, const char* username, const char* password);

void session_logout(void* s);

uid_t session_remove_user(void* s, const char* username);

#endif //USER_SESSION_H