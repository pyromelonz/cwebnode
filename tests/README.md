# Authentication DOM smoke test

Run from the project root:

```sh
./tests/run_auth_forms.sh
```

The test builds the real DOM/node renderer and login/register route handlers
against minimal mocks for the external webserver and component registry. It
checks each rendered form's action, input names/types/required attributes,
labels, submit button, and that GET renders the page while a successful POST
still calls the appropriate session function and redirects to `/index`.

AddressSanitizer and UndefinedBehaviorSanitizer are enabled; a full production
server build still requires the external libcwebui and third-party headers.
