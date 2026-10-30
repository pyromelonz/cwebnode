#!/bin/sh
# Build and exercise the real DOM constructors and login/register handlers using
# mock webserver/registry functions (the external webserver is not in this repo).
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

"${CC:-cc}" -std=gnu11 -g -O1 -fsanitize=address,undefined \
    -I"$root_dir/tests/mocks" -I"$root_dir/include" \
    "$root_dir/tests/test_auth_forms.c" \
    "$root_dir/src/user_session_functions.c" \
    "$root_dir/src/misc.c" \
    "$root_dir/src/dombase.c" \
    "$root_dir/src/web_node.c" \
    "$root_dir/src/web_node_aspects.c" \
    "$root_dir/src/wn_deliver.c" \
    "$root_dir/src/wn_component_token.c" \
    "$root_dir/src/container.c" \
    -o "$build_dir/test_auth_forms"

ASAN_OPTIONS=detect_leaks=1 "$build_dir/test_auth_forms"
