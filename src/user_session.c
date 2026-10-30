#include "user_auth.h"
#include "user_session.h"
#include "webserver_api_functions.h"
#include "user_data.h"

uid_t session_get_user(void* s) {
    #if STORE_NORMAL != NORMAL_CHECK_OK || STORE_SSL != SSL_CHECK_OK
        #error LIBCWEBUI CHANGED
    #endif

    int registerStatus = checkUserRegisterStatus(s);

    if (registerStatus == SESSION_MISMATCH_ERROR || registerStatus == NOT_REGISTERED) { 
		LOG(DBG,"session %s rejected -- register status %d",getSessionGUID(s),registerStatus);
		return 0;
	} else {
		LOG(DBG,"session %s is registered\n",getSessionGUID(s));
	}

	const char* username = getVariableAsStringP(getSessionVar(s,registerStatus,"username",1));

    uid_t user = user_get_uid( username );
	LOG(DBG,"session_get_user found uid %u",user);
	return (user_is_session_authorized(s,user)) ? user : 0;
}

uid_t session_login_user(void* s, const char* username, const char* password) {
	
	uid_t uid = user_authenticate(username, password);
	if (uid) {
		uid_t current_user = session_get_user(s);
		if (current_user) {
			LOG(MSG,"User %s still logged in, logging out and loggin in %s",user_get_pnym(current_user),username);
			session_logout(s);
		}
		user_authorize_session(s, uid);
        return uid;

	} else {
		LOG(MSG, "No matching credentials for %s in system.",username);
		return 0;
	}
}

void session_logout(void* s) {
	LOG(DBG,"logout called");
    user_unauthorize_session(s, session_get_user(s));
	if (session_get_user(s)) LOG(ALERT,"LOGOUt FAILED\n");
}

uid_t session_register_user(void* s, const char* username, const char* password) {
	
	if (user_get_uid(username)) {
		LOG(MSG,"User %s already registered, cannot register again",username);
		return 0;
	}
	
	uid_t uid = user_register(username, password);
	
	if (uid) {
		user_data_add(uid,username);
		session_login_user(s,username,password);
		return uid;
	} else {
		LOG(ALERT, "Could not register user %s",username);
		return 0;
	}
}

uid_t session_remove_user(void* s, const char* username) {
	uid_t uid = user_remove(username);
	uid_t current_user = session_get_user(s);
	if (uid) {
		if (current_user == uid) {
			session_logout(s);
		}
		LOG(MSG,"Removing user data");
		user_data_remove(uid);
	} 
	return uid;
}