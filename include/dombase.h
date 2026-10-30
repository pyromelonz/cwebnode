#ifndef DOMBASE_H
#define DOMBASE_H

#include "web_node_macro.h"

//nodes should be stk_t of web_nodes

web_node* DOM_vert( stk_t nodes);

web_node* DOM_horz(stk_t nodes);

web_node* _DOM_vert_v(web_node* elements, ...);
web_node* _DOM_horz_v(web_node* elements, ...);

#define DOM_VERT(...) _DOM_vert_v(__VA_ARGS__, 0)
#define DOM_HORZ(...) _DOM_horz_v(__VA_ARGS__, 0)




web_node* DOM_user_header(dummy_handler* s);


///// destroy destroy destroy
// application based doms

web_node* 



#endif //DOMBASE_H
