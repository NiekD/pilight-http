/*
	Copyright (C) 2018 CurlyMo & Niek

	This file is part of pilight.

	pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../core/threads.h"
#include "../action.h"
#include "../../core/options.h"
#include "../../config/devices.h"
#include "../../core/log.h"
#include "../../core/dso.h"
#include "../../core/pilight.h"
#include "../../core/http.h"
#include "../../core/common.h"
#include "http.h"


typedef struct settings_t {
	char *device;
	char *method;
	char *mimetype;
	char *param;
	char *url;
	struct settings_t *next;
} settings_t;


static char* strip(char* input) {
	int loop;
	char *output = (char*) malloc (strlen(input) + 1);
	char *dest = output;
	
	if (output)
	{
	for (loop=0; loop<strlen(input); loop++)
		if (input[loop] != ' ')
		*dest++ = input[loop];
	*dest = '\0';
	}
	return output;
}

static int checkArguments(struct rules_actions_t *obj) {
	struct JsonNode *jget = NULL;
	struct JsonNode *jpost = NULL;
	struct JsonNode *jdevice = NULL;
	struct JsonNode *jparam = NULL;
	struct JsonNode *jmimetype = NULL;
	struct JsonNode *jvalues = NULL;
	struct JsonNode *jchild = NULL;
	int nrvalues = 0, match =0;
	
	if(obj == NULL) {
		/* Internal error */
		return -1;
	}

	if(obj->arguments == NULL) {
		/* Internal error */
		return -1;
	}

	jpost = json_find_member(obj->arguments, "POST");
	jget = json_find_member(obj->arguments, "GET");
	jdevice = json_find_member(obj->arguments, "DEVICE");
	jparam = json_find_member(obj->arguments, "PARAM");
	jmimetype = json_find_member(obj->arguments, "MIMETYPE");
	
	if(jpost == NULL && jget == NULL) {
		logprintf(LOG_ERR, "http action is missing a \"GET\" or \"POST\"");
		return -1;
	}
	if(jpost != NULL && jget != NULL) {
		logprintf(LOG_ERR, "http action must contain either a \"GET\" or a \"POST\"");
		return -1;
	}	
	nrvalues = 0;
	if(jpost != NULL) {
		if((jvalues = json_find_member(jpost, "value")) != NULL) {

			jchild = json_first_child(jvalues);
			while(jchild) {
				nrvalues++;
				jchild = jchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "http action: \"POST\" only takes one argument");
			return -1;
		}
	}
	nrvalues = 0;
	if(jget != NULL) {
		if((jvalues = json_find_member(jget, "value")) != NULL) {
	
			jchild = json_first_child(jvalues);
			while(jchild) {
				nrvalues++;
				jchild = jchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "http action: \"GET\" only takes one argument");
			return -1;
		}	
	}
	nrvalues = 0;
	if(jmimetype != NULL) {
		if((jvalues = json_find_member(jparam, "value")) != NULL) {
	
			jchild = json_first_child(jvalues);
			while(jchild) {
				nrvalues++;
				jchild = jchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "http action: \"PARAM\" only takes one argument");
			return -1;
		}	
	}
	nrvalues = 0;
	if(jmimetype != NULL) {
		if((jvalues = json_find_member(jmimetype, "value")) != NULL) {
	
			jchild = json_first_child(jvalues);
			while(jchild) {
				nrvalues++;
				jchild = jchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "http action: \"MIMETYPE\" only takes one argument");
			return -1;
		}	
	} else {
		if(jpost != NULL) {
			logprintf(LOG_ERR, "http action: a \"MIMETYPE\" is required for \"POST\" requests");
			return -1;
		}
	}
	

	nrvalues = 0;
	if(jdevice != NULL) {

		if((jvalues = json_find_member(jdevice, "value")) != NULL) {
			jchild = json_first_child(jvalues);
			while(jchild) {
				nrvalues++;
				jchild = jchild->next;
			}
		}
		if(nrvalues != 1) {
			logprintf(LOG_ERR, "http action: \"DEVICE\" only takes one argument");
			return -1;
		}
		jchild = json_first_child(jvalues);
	
		struct devices_t *dev = NULL;
		if(devices_get(jchild->string_, &dev) == 0) {
			struct protocols_t *protocols = dev->protocols;
			match = 0;
			while(protocols) {
				if(strcmp(protocols->name, "generic_http") == 0) {
					match = 1;
					break;
				}
				protocols = protocols->next;
			}
			if(match == 0) {
				logprintf(LOG_ERR, "http action: \"DEVICE\" must be a generic_http device");
				return -1;
			}
		} else {
			logprintf(LOG_ERR, "http action: \"DEVICE\" device \"%s\" doesn't exist", jchild->string_);
			return -1;
		}
	}
	return 0;
}

static void callback(int code, char *data, int size, char *type, void *userdata) {
	struct settings_t *wnode = userdata;
	struct devices_t *dev = NULL;
	struct JsonNode *jobj = NULL;
	if (wnode != NULL && wnode->device != NULL && strlen(wnode->device) > 0) {
		logprintf(LOG_DEBUG, "Http action - Device: %s, Code: %i, Type: %s, Size: %i", wnode->device, code, type, size);
		if(devices_get(wnode->device, &dev) == 0) {
			if(size > 290) {
				logprintf(LOG_NOTICE, "http action response size %i is too big (limit is 290), response truncated", size);
				data[291] = '\0';
				logprintf(LOG_DEBUG, "truncated response to: \"%s\"", data);
			}		
			jobj = json_mkobject();
			if(size == 0) {
				json_append_member(jobj, "data", json_mkstring(""));				
			} else {
				json_append_member(jobj, "data", json_mkstring(data));
			}
			json_append_member(jobj, "size", json_mknumber(size, 0));
			if (type == NULL) {
				json_append_member(jobj, "mimetype", json_mkstring(""));
			} else {
				json_append_member(jobj, "mimetype", json_mkstring(type));			
			}
			json_append_member(jobj, "code", json_mknumber(code, 0));

			if(pilight.control != NULL) {
				pilight.control(dev, "ready", json_first_child(jobj), ACTION);
				json_delete(jobj);
			}	
		} else {
			logprintf(LOG_DEBUG, "http action succeeded with no \"DEVICE\" defined, response (if any) discarded");			
		}
	}
	if(code == 200) {
		logprintf(LOG_DEBUG, "http action calling \"%s\" succeeded, received \"%s\"", wnode->url, data);
	} else {
		logprintf(LOG_NOTICE, "http action calling \"%s\" failed (%i)", wnode->url, code);
		logprintf(LOG_NOTICE, "data: %s, size: %d, mimetype: %s", data, size, type);
		 
	}
	FREE(wnode->url);
	if (wnode->device != NULL) {
		FREE(wnode->device);
	}
	if (wnode->mimetype != NULL) {
		FREE(wnode->mimetype);
	}
	if (wnode->param != NULL) {
		FREE(wnode->param);
	}

	FREE(wnode);
	wnode = NULL;
}

static void *thread(void *param) {
	struct rules_actions_t *pth = (struct rules_actions_t *)param;
	struct settings_t *wnode = MALLOC(sizeof(struct settings_t));
	struct JsonNode *arguments = pth->arguments;
	struct JsonNode *jpost = NULL;
	struct JsonNode *jget = NULL;
	struct JsonNode *jparam = NULL;
	struct JsonNode *jdevice = NULL;
	struct JsonNode *jmimetype = NULL;
	struct JsonNode *jvalues1 = NULL;
	struct JsonNode *jvalues2 = NULL;
	struct JsonNode *jvalues3 = NULL;
	struct JsonNode *jvalues4 = NULL;
	struct JsonNode *jvalues5 = NULL;
	struct JsonNode *jval3 = NULL;
	struct JsonNode *jval4 = NULL;
	struct JsonNode *jval5 = NULL;
	struct JsonNode *jobj = NULL;

	struct devices_t *dev = NULL;

	action_http->nrthreads++;
	
	wnode->device = NULL;
	wnode->url = NULL;
	wnode->mimetype = NULL;

	jpost = json_find_member(arguments, "POST");
	jget = json_find_member(arguments, "GET");
	jdevice = json_find_member(arguments, "DEVICE");
	jparam = json_find_member(arguments, "PARAM");
	jmimetype = json_find_member(arguments, "MIMETYPE");

	if(jpost != NULL) {
		if((jvalues1 = json_find_member(jpost, "value")) != NULL) {
			jpost = json_find_element(jvalues1, 0);
		}
	}
	if(jget != NULL) {
		if((jvalues2 = json_find_member(jget, "value")) != NULL) {
			jget = json_find_element(jvalues2, 0);
		}
	}
	if(jparam != NULL) {
		if((jvalues3 = json_find_member(jparam, "value")) != NULL) {
			jval3 = json_find_element(jvalues3, 0);
		}
		if(jval3 != NULL && jval3->tag == JSON_STRING) {
			
			if((wnode->param = MALLOC(1024)) == NULL) {
//			if((wnode->param = MALLOC(strlen(jval3->string_) + 1)) == NULL) {
				fprintf(stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(wnode->param, jval3->string_); 
			str_replace(" ", "%20", &wnode->param);
		}
	}
	if(jmimetype != NULL) {
		if((jvalues4 = json_find_member(jmimetype, "value")) != NULL) {
			jval4 = json_find_element(jvalues4, 0);
			if((wnode->mimetype = MALLOC(strlen(jval4->string_) + 1)) == NULL) {
				logprintf(LOG_ERR, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(wnode->mimetype,jval4->string_) ;
		}
	}
	if(jdevice != NULL) {
		if((jvalues5 = json_find_member(jdevice, "value")) != NULL) {
			jval5 = json_find_element(jvalues5, 0);
			if((wnode->device = MALLOC(strlen(jval5->string_) + 1)) == NULL) {
				logprintf(LOG_ERR, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			strcpy(wnode->device,jval5->string_) ;
			if (wnode->device != NULL && strlen(wnode->device) > 0) {
				if(devices_get(wnode->device, &dev) == 0) {
					if(pilight.control != NULL) {
						pilight.control(dev, "busy", NULL, ACTION);
						json_delete(jobj);
					}
				}
			}
		}
	}

	if((wnode->url = MALLOC(1024)) == NULL) {
		logprintf(LOG_ERR, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	
	
	
	if(jpost != NULL && jpost->tag == JSON_STRING) { //POST REQUEST
		strcpy(wnode->url, strip(jpost->string_)); 

		http_post_content(wnode->url, wnode->mimetype, wnode->param, callback, wnode);
	} else {
		if(jget != NULL && jget->tag == JSON_STRING) { //GET REQUEST
			if(wnode->param != NULL) {
				sprintf(wnode->url, "%s?%s", strip(jget->string_), wnode->param);
			} else {
				strcpy(wnode->url, strip(jget->string_));
			}
			http_get_content(wnode->url, callback, wnode);
		} 
	}
	logprintf(LOG_DEBUG, "http action called %s", wnode->url);

	action_http->nrthreads--;

	return (void *)NULL;
}

static int run(struct rules_actions_t *obj) {
	pthread_t pth;
	threads_create(&pth, NULL, thread, (void *)obj);
	pthread_detach(pth);
	return 0;	 
}
 
#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void actionHttpInit(void) {
	event_action_register(&action_http, "http");

	options_add(&action_http->options, 'a', "POST", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_STRING, NULL, NULL);
	options_add(&action_http->options, 'b', "GET", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_STRING, NULL, NULL);
	options_add(&action_http->options, 'd', "DEVICE", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_STRING, NULL, NULL);
	options_add(&action_http->options, 'm', "MIMETYPE", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_STRING, NULL, NULL);
	options_add(&action_http->options, 'p', "PARAM", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_STRING, NULL, NULL);

	action_http->run = &run;
	action_http->checkArguments = &checkArguments;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "http";
	module->version = "0.3";
	module->reqversion = "7.0";
	module->reqcommit = "87";
}

void init(void) {
	actionHttpInit();
}
#endif
