# pilight-http
This repository contains an addon enabling http requests from your rules and handling of the result for pilight 8.

This addon will replace the webswitch protocol I published earlier.

One action:

* http, enables you to call remote services via HTTP(S) GET or POST.

One protocol:

* generic_http_result, which can indicate the state of the http action and store its results.


## Installation
All addons can be compiled as modules to be installed in the appropriate pilight folders. 
Always use pilight development V7, which is the code base for pilight V8.
