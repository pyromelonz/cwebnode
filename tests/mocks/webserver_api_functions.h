#ifndef WEBSERVER_API_FUNCTIONS_H
#define WEBSERVER_API_FUNCTIONS_H
#include <stddef.h>
typedef struct dummy_handler { char *output; size_t length, capacity; const char *post_data; } dummy_handler;
typedef enum { WS_FILE_TYPE_HTML=1, WS_FILE_TYPE_CUSTOM=2, WS_FILE_TYPE_PLAIN=3 } WS_FILE_TYPES;
void printHTML(dummy_handler*, const char*, ...);
size_t getPostSize(dummy_handler*);
const char* getPostData(dummy_handler*);
#endif
