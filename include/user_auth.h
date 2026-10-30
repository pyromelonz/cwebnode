#ifndef USER_AUTH_H
#define USER_AUTH_H

#define USERNAME_MAX_SIZE 16

#include "log.h"
#include "types.h"


uid_t	user_get_uid( const char* username);

//returns uid [1,2³¹-1] on sucessful registration
uid_t	user_register( const char* username, const char* password );
uid_t	user_remove( const char* username );

//returns uid [1,2³¹-1] on sucessful authentication
uid_t	user_authenticate( const char* username, const char* password );

err_t	auth_reg_load(const char* path),
	auth_reg_save(const char* path);

void auth_init(), auth_term(), auth_reg_print();

//


#endif //USER_AUTH_H
