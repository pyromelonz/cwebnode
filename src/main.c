#include "log.h"
//#include "cfg.h"
#include <stdio.h>
#include "misc.h"
#include <string.h>

#include "sites.h"

#include <signal.h>
#include <stdlib.h>

#include "user_auth.h"
#include "user_data.h"

#include "tree_misc.h"
#include "dombase.h"
#include "web_node_macro.h"
#include "sites_macro.h"
#include "wn_component_registry.h"

void init() {
	log_init();
	auth_init();
	user_init();
	comp_reg_init();
}

DEFINE_WEB_RESOURCE(test_now, USER | ADMIN | GUEST) {
}

void post_init() {
	sites_init();
	static const unsigned long long maxpost = 0xFFFFFFFFFFULL;
	WebserverConfigSetInt("max_post_size",maxpost);
	printf("setintmaxpostsize %llu returned %d", maxpost, 0);
}

void sigint_handler(int) {
	signal(SIGINT,SIG_IGN);
	WebserverShutdownHandler();
	signal(SIGINT,exit);
}

DEFINE_WEB_RESOURCE(shutdown_dev,GUEST | USER) {
	WebserverShutdownHandler();
}

void term() {
	comp_reg_term();
	auth_term();
	sites_term();
	user_term();

#ifdef DEBUG_MEM_V
	mem_term();
#elif defined(DEBUG_MEM)
	LOG(DBG,"Remaining Allocations: %u",mem_get_remaining_allocations());
#endif
	log_term();
}
#include "entropy.h"
int main() {

	init();

	{
		FILE* v = fopen("var/version.txt","r");
		char buf[64];
		if (v && fgets(buf, sizeof(buf), v)) {
			buf[sizeof(buf)-1] = 0;
			LOG(MSG,"Webserver version: %s\n",buf);
		} else{
			LOG(FATAL,"Malformed installation or wrong working directory - couldn't read version file");
			term();
			return 1;
		}
	}

	if (!WebserverInit()) {

		signal(SIGINT,sigint_handler);
		signal(SIGTERM,sigint_handler);
		
		WebserverConfigSetInt("port",8080);
		#ifdef USE_SSL
			WebserverConfigSetInt("ssl_port",SSL_PORT);
			WebserverConfigSetText("ssl_key_file", SSL_KEY_FILE);
			WebserverConfigSetText("ssl_key_file_password", "");
			WebserverConfigSetText("ssl_dh_file", SSL_DH_PARAM_FILE);
			WebserverConfigSetText("ssl_ca_list_file", SSL_CA_LIST_FILE);
		#endif
		ws_add_dir("","www",0,0);
		ws_add_dir("rsc","rsc",0,0);
		post_init();

		WebserverStart();
	}
	
	term();
	return 0;

}


