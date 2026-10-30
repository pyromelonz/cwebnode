#include "log.h"
#include <stdio.h>
#include <stdarg.h>

#include <string.h>
#include <stdint.h>

#include "config.h"

#if !(defined BUFFER_SIZE) || BUFFER_SIZE < 120
#define BUFFER_SIZE 120
#endif

static FILE* log_file = 0;

static char buffer[BUFFER_SIZE];

static const char status_str[N_LOG_T][6] = {
	#define X(a) #a,
		LOG_T_LIST
	#undef X
};

const char* get_status_str(err_t status) {
	return status_str[status];
}

static err_t max_err = 0;

err_t log_get_error() {
	return max_err;
}

char log_init() {
#ifdef LOG_FILE
	if (log_file || !(log_file = fopen(LOG_FILE,"a"))) {
		log_file = stdout;
		printf("COULD NOT OPEN LOG FILE WITH PATH %s\n",LOG_FILE);
	}
#else
	log_file = stdout;
#endif
	return 0;
}

void log_term() {
	#ifdef LOG_FILE
	if (log_file && log_file != stdout) {
		fclose(log_file);
		log_file = 0;
	}
	#endif
}

static void submit_buffer() {
	fwrite(buffer,1,strlen(buffer),log_file);
	fflush(log_file);
}

void log_vprintf(const char* fmt, va_list argptr) {
	vsnprintf(buffer,BUFFER_SIZE,fmt,argptr);
	submit_buffer();
}

void log_printf(const char* fmt, ...) {
	va_list argptr;
	va_start (argptr, fmt);
	log_vprintf(fmt,argptr);
	va_end(argptr);
}


void log_msg(err_t status, const char* const fmt, ...) {
	#if !defined DEBUG || defined NDEBUG
	return;
	#endif
	{
		log_printf("%s: \t",get_status_str(status));
		va_list argptr;
		va_start (argptr,fmt);
		log_vprintf(fmt, argptr);
		fputc('\n',log_file);
		va_end(argptr);
	}
	max_err = (status > max_err) * status + (status <= max_err) * max_err;
}
