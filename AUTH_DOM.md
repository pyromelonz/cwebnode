# Login and registration as reusable web nodes

The authentication forms previously shown as standalone HTML are now
constructors declared in `include/dombase.h`:

| Constructor | Returns |
| --- | --- |
| `DOM_login_form()` | A `<form>` posting `username` and `password` to `/login` |
| `DOM_register_form()` | A `<form>` posting `username` and `password` to `/register` |
| `DOM_login_page()` | The login heading plus `DOM_login_form()` |
| `DOM_register_page()` | The registration heading plus `DOM_register_form()` |

All constructors return a new `web_node*` tree that the caller owns and can
attach to another parent. When done, destroy the containing root node as usual.

Example:

```c
web_node* root = wn_wrap_html_root(DOM_login_page());
wn_deliver(s, root);
delete_web_node(root);
```

`src/user_session_functions.c` already uses the page constructors when GET
requests reach `/login` and `/register`. A form POST still calls the existing
session login or registration functions and redirects to `/index` on success.
The navigation in `DOM_user_header` now links to those dynamic routes.

The HTML node renderer supports `h1`, `form`, `label`, `input`, and `button`;
`input` is rendered without an invalid closing tag. Login and registration
fields have distinct DOM IDs, but retain the `username` and `password` names
required by the existing POST parser.

Run `./tests/run_auth_forms.sh` for sanitizer-enabled DOM and handler smoke
checks. These tests mock only the external webserver API and registry; the full
server build still depends on the separately provided libcwebui components.
