# pilight-http
This repository contains an addon enabling http requests from your rules and handling of the result for pilight 8.1.

This addon will replace the webswitch protocol I published earlier.

# http action
The http action can be used in pilight rules to send HTTP POST or GET requests, either with or without parameters. 

# generic_http protocol
The state and the result of the request can optionally be stored in a generic_http device.

Configuration:
```
"http_response": {
			"protocol": [ "generic_http" ],
			"id": [{
				"id": 1
			}],
			"mimetype": "text/html",
			"size": 4,
			"code": 200,
			"data": "test",
			"state": "ready"
		},
```
* Mimetype is the mimetype returned by the http request
* Size is the number of bytes of the data returned
* Code is the http code resulting from the http request (200 means request succeeded)
* Data is the payload returned
* State is the state of the http request

Note the difference between state and code. State doesn't tell if the http request was succesful, it just tells that the request is busy or ready. If state is "ready", you can use code to tell if the request succeeded or not.

If you want to make other rules act upon the result of the http(s) request, you can use the state change of the generic_http device to trigger those rules, just like you can do with the state of other devices. 
The state will be either "busy" or "ready". The state changes to "busy"  when the http action starts and back to "ready" when the http action finishes. When the state is "ready", the generic_http device contains values related to the latest http request.


## Usage
```
IF ... THEN http GET|POST <url> [PARAM <parameters>] [MIMETYPE <mimetype>] [DEVICE <generic_http device>]
```
GET or POST  with url are mandatory, PARAM, MIMETYPE and DEVICE are optional.
Url and parameters can be strings or device values or combinations of both.

DEVICE must be a generic_http device.

*N.B. A trailing slash (/) is required for the url if it refers to a path as shown in the examples below.*

Some examples

```
IF ... THEN http GET 'http://192.168.2.10/'

IF ... THEN http GET http://url/path/page.html

IF ... THEN http POST 'https://192.168.2.10/' 

IF ... THEN http GET 'http://192.168.2.10/test.cgi'

IF ... THEN http GET 'http://192.168.2.10/' PARAM command=start&reply=yes

IF ... THEN http 'GET http://192.168.2.10/' DEVICE myresult

IF ... THEN http GET 'http://192.168.2.10/' PARAM 'c=' . mysensor.state DEVICE myresult

IF ... THEN http GET 'http://192.168.2.10/' PARAM command=start MIMETYPE 'text/html' DEVICE myresult

IF myresult.state == ready THEN ...

IF myresult.state == ready AND myresult.code == 200 THEN ...
```

Note that in pilight 8.1 you have to put strings containing dots (.) between quotes and that strings and device values need to be concatenated using the concatenation character (.) 

## Installation
All addons can be compiled as modules to be installed in the appropriate pilight folders. 
Install pilight manually from the staging branch, which is the code base for pilight V8.1.

Make sure you are in the pilight directory.
Clone this repository or download the individual source files to your system.
Copy the source files to the proper piligh directory.
Then compile them as modules.
```
service pilight stop

cd /home/pi/pilight/

cp /<path to downloaded source files>/generic_http.c libs/pilight/protocols/generic/

cp /<path to downloaded source files>/generic_http.h libs/pilight/protocols/generic/

cp /<path to downloaded source files>/http.c libs/pilight/events/actions/

cp /<path to downloaded source files>/http.h libs/pilight/events/actions/

gcc -fPIC -shared lib/pilight/protocols/generic/generic_http.c -Ilibs/pilight/protocols/ -Iinc/ -o generic_http.so -DMODULE=1
 
cp generic_http_result.so /usr/local/lib/pilight/protocols/
 
gcc -fPIC -shared libs/pilight/events/actions/http.c -Ilibs/pilight/events/ -Iinc/ -o http.so -DMODULE=1
 
cp http.so /usr/local/lib/pilight/actions/ 

service pilight start
 
```
