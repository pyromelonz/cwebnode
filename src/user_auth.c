#include "user_auth.h"
#include <stdint.h>
#include <stdio.h>
#include "tree_misc.h"
#include "misc.h"
#include "entropy.h"

#include "config.h"

#include <string.h>

#include "rb.h"
#include <pthread.h>

pthread_mutex_t tree_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifndef SCRAMBLEKEY
#define SCRAMBLEKEY 0xf2641923
#endif


#define PNYM_S USERNAME_MAX_SIZE / 4
#define PWOD_S 9
#define UID_S 1

#define AUTH_N_ITEMS (PNYM_S+PWOD_S+UID_S)

typedef struct auth_t {
	uint32_t pnym[PNYM_S];
	uint32_t pwod[PWOD_S];
	uid_t uid;
} auth_t;

auth_t* login_to_new_auth(const char* username, const char* password) {
	auth_t* a = sw_malloc(sizeof(auth_t));

	{
		unsigned char un = strlen(username), pw = strlen(password);
		if (!un || un > PNYM_S * 4 || !pw || pw > PWOD_S * 4 ) {
			LOG(WARN,"Tried to create user with invalid credentials:\t un: %s\t pw:%s",username,*password ? password : "(empty)");
			return (void*) 0;
		}
	}

	strncpy((void*)(a->pnym),username,PNYM_S*4);
	strncpy((void*)(a->pwod),password,PWOD_S*4);

	return a;
}


static rbtree* auth_reg;

uid_t user_get_uid(const char* username) {
	auth_t* tmp = login_to_new_auth(username,"_");
	rbnode* fnd = rb_find(auth_reg, tmp);
	sw_free(tmp);
	uid_t ret = fnd ? ((auth_t*) fnd->data)->uid : 0;
	return ret;
}

int auth_comparison_func( const void* _a, const void* _b ) {
	const uint32_t *a = ((auth_t*)_a)->pnym, *b = ((auth_t*)_b)->pnym;
	for (unsigned char i = 0; i<PNYM_S; i++) {
		if (a[i] == b[i]) continue;
		return (a[i]<b[i])-(a[i]>b[i]);
	}
	return 0;
}

static FILE* auth_file;

typedef struct {
		uint32_t uid;
		auth_t* ret;
} auth_by_uid_helper_struct;

int auth_by_uid_helper(void* data, void* cookie) {
	auth_by_uid_helper_struct* const c = cookie;
	if (c->uid == ((auth_t*)data)->uid) {
		c->ret = data;
	}
	return 0;
}

auth_t* auth_by_uid(uint32_t uid) {
	auth_by_uid_helper_struct helper;
	helper.ret = 0;
	helper.uid = uid;
	RB_APPLY(auth_reg, auth_by_uid_helper, &helper,INORDER );
	return helper.ret;
}

static void auth_print(void* user) {
	log_msg(DBG,"User:");
	log_printf("username: %s\npassword: %s\n",(char*)((auth_t*)user)->pnym,(char*)((auth_t*)user)->pwod);
	log_printf("uid : %u\n",((auth_t*)user)->uid);
}

static int auth_reg_print_h(void* user,void*) {
	auth_print(user);
}

void auth_reg_print() {
	RB_APPLY(auth_reg,auth_reg_print_h,0,PREORDER);
}

err_t auth_reg_save(const char* path) {
	err_t r = tree_save(path, auth_reg, sizeof(auth_t));
	if (r) {
		log_msg(CRIT,"Couldn't save user registry to %s",path);
	} else {
		log_msg(DBG,"Saved user registry to %s",path);
	}
	return r;
}

err_t auth_reg_load(const char* path) {
	err_t r = tree_load(path, auth_reg, sizeof(auth_t));
	if (r) {
		log_msg(CRIT,"Couldn't load user registry from %s",path);
	} else {
		log_msg(DBG,"Loaded user registry from %s",path);
	}
	return r;
}

p_sw_free(auth); //for auth_sw_free

void auth_init() {
	log_msg(DBG,"Creating auth_reg rbtree");
	auth_reg = rb_create(auth_comparison_func,auth_sw_free);
#ifdef LOAD_USERS
	tree_load(USER_REG_FILE, auth_reg, sizeof(auth_t));
#endif
}

void auth_term() {
	#ifdef SAVE_USERS
	tree_save(USER_REG_FILE, auth_reg, sizeof(auth_t));
	#endif
	if (auth_reg){
		log_msg(DBG,"Destroying auth_reg rbtree");
	 	rb_destroy( auth_reg );
	}
}

uid_t user_register( const char* username, const char* password ) {
	auth_t* a = login_to_new_auth(username,password);

	if (!a) {
		return 0;
	}
	
	pthread_mutex_lock(&tree_mutex);
	rbnode* user = rb_find(auth_reg, a);

	uid_t ret;
	
	if (user) {
		LOG(DBG, "User <%s> already exists.",username);
		sw_free(a);
		ret = 0;
	}
	else { //register new user
		do {
			a->uid = random_u32();
		} while (!a->uid || auth_by_uid(a->uid)); //assign unique uid

		LOG(DBG,"New user: %s",username);
		ret = ((auth_t*)rb_insert(auth_reg, a)->data)->uid;
	}
	pthread_mutex_unlock(&tree_mutex);
	return ret;
}

uid_t user_remove( const char* username ) {
	auth_t* a = login_to_new_auth(username,"_");
	uid_t ret = 0;
	pthread_mutex_lock(&tree_mutex);
	{
		rbnode* entry = rb_find(auth_reg, a);
		if (entry) {
			ret = ((auth_t*)entry->data)->uid;
			rb_delete(auth_reg, entry, 0);
			LOG(MSG,"Removed user %s",username);
		} else {
			LOG(MSG,"User %s doesn't exist!",username);
			ret = 0;
		}
	}
	pthread_mutex_unlock(&tree_mutex);
	sw_free( a);
	return ret;
}

uid_t user_authenticate( const char* username, const char* password ) {
	
	auth_t* a = login_to_new_auth(username,password);
	uid_t ret = 0;
	{
		rbnode* entry = rb_find(auth_reg, a);
		if (entry) {
			ret = ((auth_t*)entry->data)->uid;
			for (int i = 0; i<5; i++) {
				if (((auth_t*)a)->pwod[i] != ((auth_t*)(entry->data))->pwod[i]) {
					ret = 0;
				}
			}
		} else {
			LOG(MSG,"User %s doesn't exist!",username);
			ret = 0;
		}
	}

	sw_free( a);
	return ret;
}

