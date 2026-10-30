#include "sites.h"
#include "sites_macro.h"
#include "misc.h"
#include "rb.h"
#include <string.h>

#include "user_session.h"
#include "user_data.h"



WS_FILE_TYPES redirect(dummy_handler* s, const char* url) {
    ws_set_response_status(s, 302);
    ws_set_response_header(s, "Location", url);
	ws_set_response_header(s, "Content-Length", "0");
	ws_set_response_header(s, "Connection", "Close");
	return WS_FILE_TYPE_CUSTOM;
}

int _web_resource_cmp(const void* _a, const void* _b) {
	const web_resource *a = _a, *b = _b;
	return strcmp(a->url,b->url);
}

rbtree* web_resources = 0;

WS_FILE_TYPES web_resource_dispatcher(dummy_handler* s, const char* url) {
	web_resource wrd;
	wrd.url = url;
	rbnode* node = rb_find(web_resources, &wrd);
	if (!node) {
		LOG(WARN,"No web resource found for url %s", url);
		printHTML(s,"No such resource\n");
		return WS_FILE_TYPE_PLAIN;
	}

	web_resource* wr = (web_resource*) node->data;

	uid_t uid = session_get_user(s);
	LOG(DBG,"wr groups: %u, user groups: %u", wr->access_groups, uid ? (user_data_get(uid)->groups) : GUEST);
	if (wr->access_groups & (uid ? (user_data_get(uid)->groups) : GUEST)) {
		LOG(DBG,"User %s authorized for resource %s", user_get_pnym(uid), url);
		return wr->func(s);
	}
	LOG(DBG,"User %s not authorized for resource %s", uid ? user_get_pnym(uid) : "Guest", url);

	printHTML(s,"Access Denied\n");
	return WS_FILE_TYPE_HTML;
}

err_t web_resource_add_func (specific_url_handler func, const char* url, unsigned char indexed, USER_GROUPS access_groups) {
	web_resource* wr = sw_malloc (sizeof(web_resource));
	wr->url = url;
	rbnode* existing = rb_find(web_resources, wr);
	if (existing) {
		LOG(WARN,"Web resource with url %s already exists, aborting duplicate resource addition", url);
		return WARN;
	}
	wr->func = func;
	wr->access_groups = access_groups;
	rb_insert(web_resources, wr);
	LOG(DBG,"Added web resource with url %s", url);
	return WS_SUCCESS;
}

p_sw_free(wrtree);

err_t web_resource_add_init(specific_url_handler func, const char* url, unsigned char indexed, USER_GROUPS access_groups) {
	web_resources = rb_create(_web_resource_cmp, wrtree_sw_free);
	web_resource_add = web_resource_add_func;
	return web_resource_add(func, url, indexed, access_groups);
}

err_t (*web_resource_add)(specific_url_handler func, const char* url, unsigned char indexed, USER_GROUPS access_groups) = web_resource_add_init;

int wr_tree_init_func(void* data, void* cookie) {
	char * urlstr = sw_strdup(((web_resource*)data)->url);
	ws_register_url_function(urlstr, web_resource_dispatcher);
	sw_free(urlstr);
	return 0;
}

void sites_init() {
	{
		web_resource* wrd;
	#define X(a) wrd = &WR_DAT_NAME(a); \
		web_resource_add(wrd->func, wrd->url, 0, wrd->access_groups);
		WEB_RESOURCE_LIST
	#undef X
	}
	RB_APPLY(web_resources,wr_tree_init_func,0,INORDER);
}

void sites_term() {
	rb_destroy(web_resources);
}

#include <stdio.h>

DEFINE_WEB_RESOURCE(upload_file, USER | GUEST) {
	int post_file_count = getFileCount(s);
	printHTML(s,"Received %d files:<br>\n", post_file_count);
	for (int f = 0; f < post_file_count; f++) {
		const char* fname = getFileName(s, f);
		size_t fsize = getFileSize(s, f);
		printHTML(s,"File %llu: %s (%zu bytes)<br>\n", f, fname, fsize);
		char fname_buf[256];
		snprintf(fname_buf, sizeof(fname_buf), "var/uploads/%s", fname);
		FILE* fd = fopen(fname_buf, "wb");
		if (fd) {
			fwrite(getFileData(s,f),1,getFileSize(s,f), fd);
			fclose(fd);
			printHTML(s,"Saved to %s<br>\n", fname_buf);
		} else {
			printHTML(s,"Failed to save file %s<br>\n", fname);
		}
	}
	return WS_FILE_TYPE_HTML;
}

static unsigned n_where = 0;
DEFINE_WEB_RESOURCE(where, USER | GUEST) {
	printf("n_where: %u",++n_where);
	return redirect(s,"/which_destination.html");
}

DEFINE_WEB_RESOURCE(edited, USER | GUEST) {
        return redirect(s,"/insanity_skeleton.txt");
}

#include "container.h"
static stk_t msgstk =0;

DEFINE_WEB_RESOURCE(the_sky_opens, USER | GUEST) {
	unsigned size = getPostSize(s);
char* msg;
	unsigned read = read_post_data(getPostData(s),"message",&msg);
	if (read == 1) {
		printf("read a message: %s\n",msg);
		msgstk = stack_push(msgstk,sw_strdup(msg));
		return redirect(s,"post_message.html");
	} else {
		return 0;
	}
}

#include "web_node_macro.h"

DEFINE_WEB_RESOURCE(view_messages, USER | GUEST) {
	for (stk_t i = msgstk; i; i=i->next) {
		printHTML(s,"%s\n",i->data);
	}	
	return WS_FILE_TYPE_HTML;
}


