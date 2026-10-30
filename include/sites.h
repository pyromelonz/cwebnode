#ifndef SITES_H
#define SITES_H

#include "log.h"

#include "webserver_api_functions.h"

WS_FILE_TYPES redirect (dummy_handler* s, const char* url);

typedef WS_FILE_TYPES (*specific_url_handler)(dummy_handler* s);

extern err_t (*web_resource_add)(specific_url_handler func, const char* url, unsigned char indexed, USER_GROUPS access_groups);

void sites_init(), sites_term();

#endif //SITES_H
