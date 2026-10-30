#ifndef CONFIG_H
#define CONFIG_H


#ifndef SCRAMBLEKEY
//#define SCRAMBLEKEY 0xf2641923
#endif

#define ROOT_USERNAME "admin"
#define ROOT_PASSWORD "admin"

#define LIBCWEBUI_INCLUDE_PATH "lib/src/libcwebui/lib/include/webserver.h"

//#define SESSION_TESTING

#ifndef DEBUG
#define DEBUG
#endif

#define PERSISTENT_USERS

#define USER_REG_FILE "var/user_reg.dat"
#define USER_DATA_FILE "var/user_data.dat"

//#define PERSISTENT_USERS   //uncomment to save user data and registrations between runs

#ifdef USE_SSL
    #define SSL_KEY_FILE "/home/dev/ssl/fullpriv.pem"
    #define SSL_KEY_FILE_PASSWORD ""
    #define SSL_DH_PARAM_FILE "/home/dev/ssl/dhparam.pem"
    #define SSL_CA_LIST_FILE "/home/dev/ssl/rootCA.pem"
    #define SSL_PORT 8443
    #if !defined(SSL_KEY_FILE) || !defined(SSL_DH_PARAM_FILE) || !defined(SSL_CA_LIST_FILE)
        #error "USE_SSL defined but SSL_KEY_FILE, SSL_DH_FILE or SSL_CA_LIST_FILE not defined"
    #endif
#endif

#ifndef LOG_VERBOSITY
#define LOG_VERBOSITY DBG
#endif

#ifdef PERSISTENT_USERS
    #define LOAD_USERS
    #define SAVE_USERS
#endif

#if (defined(LOAD_USERS) || defined(SAVE_USERS)) && (!defined(USER_DATA_FILE) || !defined(USER_REG_FILE))
    #error "can't LOAD or SAVE users without defining USER_DATA_FILE and USER_REG_FILE"
#endif

#ifndef SSL_PORT
    #define SSL_PORT 443
#endif

#endif //CONFIG_H
