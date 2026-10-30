#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint32_t uid_t;


#define USER_GROUP_LIST \
	X(GUEST) \
	X(USER)  \
	X(ADMIN)

typedef enum {
	#define X(g) _NUM_##g,
	USER_GROUP_LIST
	#undef X
	USER_GROUP_COUNT
} USER_GROUP_NUM;

typedef enum {
	#define X(g) g = (1<<_NUM_##g),
	USER_GROUP_LIST
	#undef X
} USER_GROUPS;

#define X(a) a |
static const uint32_t ALL_USER_GROUPS = USER_GROUP_LIST 0;
#undef X

#if USER_GROUP_COUNT > 32
	#error "Too many user groups defined, change group storage type"
#endif

#define LOG_T_LIST \
	X(OK)   \
	X(MEM)  \
	X(DBG)  \
	X(ALERT)\
	X(MSG)  \
	X(WARN) \
	X(CRIT) \
	X(FATAL)

typedef enum {
#define X(a) a,
	LOG_T_LIST
#undef X
	N_LOG_T
} err_t;

#endif //TYPES_H