# pilight-http
This repository contains an addon enabling http requests from your rules and handling of the result for pilight 8.

This addon will replace the webswitch protocol I published earlier.

# http action
The http action can be used in pilight rules to send HTTP POST or GET requests, either with or without parameters. 

# generic_http_result protocol
The state and the result of the request can optionally be stored in a generic_http_result device.

Configuration:
```
"http_response": {
			"protocol": [ "generic_http_result" ],
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

If you want to make other rules act upon the result of the http(s) request, you can use the state change of the generic_http_device to trigger those rules, just like you can do with the state of other devices. 
The state will be either "busy" or "ready". The state changes to "busy"  when the http action starts and back to "ready" when the http action finishes. When the state is "ready", the generic_http_result device contains values related to the latest http request.


## Usage
```
IF ... THEN http GET|POST <url> [PARAM <parameters>] [MIMETYPE <mimetype>] [DEVICE <generic_http_result device>]
```
GET or POST  with url are mandatory, PARAM, MIMETYPE and DEVICE are optional.
Url and parameters can be strings or device values or combinations of both.

DEVICE must be a generic_http_result device.

*N.B. A trailing slash (/) is required for the url if it refers to a path as shown in the examples below.*

Some examples

```
IF ... THEN http GET http://192.168.2.10/test.cgi

IF ... THEN http POST https://192.168.2.10/ 

IF ... THEN http GET http://192.168.2.10/ PARAM command=start&reply=yes

IF ... THEN http GET http://192.168.2.10/ DEVICE myresult

IF ... THEN http GET http://192.168.2.10/ PARAM c= mysensor.state DEVICE myresult

IF ... THEN http GET http://192.168.2.10/ PARAM command=start MIMETYPE text/html DEVICE myresult

IF myresult.state == ready THEN ...

IF myresult.state == ready AND myresult.code == 200 THEN ...
```
Note the space between the "=" sign and the device variable names in the examples. These spaces are required for the eventing system to recognize the device variable (or function). 
The action will automaticly remove all spaces from both url and parameters before the request is sent. If you need to retain certain spaces in the parameters you can replace each one of them with %20

## Installation
All addons can be compiled as modules to be installed in the appropriate pilight folders. 
Always use pilight development V7, which is the code base for pilight V8.

Clone this repository or download the individual source files to your system.
Then compile them as modules. Note that pilight develoment V7 is required for this.
```
service pilight stop

gcc -fPIC -shared <folder containing source files>/generic_http_result.c -Ilibs/pilight/protocols/ -Iinc/ -o generic_http_result.so -DMODULE=1
 
cp generic_http_result.so /usr/local/pilight/protocols/
 
gcc -fPIC -shared <folder containing source files>/http.c -Ilibs/pilight/events/ -Iinc/ -o http.so -DMODULE=1
 
cp generic_http.so /usr/local/pilight/actions/ 

service pilight start
 
```
