/*
	Copyright (C) 2017 CurlyMo & Niek

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
#include <math.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "generic_http_result.h"

static void createMessage(int id, int state, char *data, int size, int code, char* mimetype) {
	generic_http_result->message = json_mkobject();
	json_append_member(generic_http_result->message, "id", json_mknumber(id, 0));
	json_append_member(generic_http_result->message, "data", json_mkstring(data));
	json_append_member(generic_http_result->message, "size", json_mknumber(size, 0));
	json_append_member(generic_http_result->message, "code", json_mknumber(code, 0));
	json_append_member(generic_http_result->message, "mimetype", json_mkstring(mimetype));
		if(state == 1) {
		json_append_member(generic_http_result->message, "state", json_mkstring("busy"));
	} else {
		json_append_member(generic_http_result->message, "state", json_mkstring("ready"));
	}
}

static int createCode(JsonNode *code) {
	char *output = json_stringify(code, NULL);
		logprintf(LOG_INFO, "Generic-http: code=%s", output);

	int id = -1, rstate = -1;
	char *rdata = "", *rmimetype = "";
	double itmp = 0, rsize = 0, rcode = 0 ;

	if(json_find_number(code, "id", &itmp) == 0) {
		id = (int)round(itmp);
	}

	if(json_find_number(code, "busy", &itmp) == 0) {
		rstate=1;		
	} else if(json_find_number(code, "ready", &itmp) == 0) {
		rstate=0;		
	}

	json_find_string(code, "data", &rdata);
	json_find_string(code, "mimetype", &rmimetype);
	json_find_number(code, "size", &rsize);
	json_find_number(code, "code", &rcode);

	if(id == -1 || state == -1) {
		logprintf(LOG_ERR, "generic_http_result: insufficient number of arguments");
		return EXIT_FAILURE;
	} else {
		createMessage(id, rstate, rdata, rsize, rcode, rmimetype);
	}
	return EXIT_SUCCESS;

}

static void printHelp(void) {
	printf("\t A generic http device can only be controlled by the http action.");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void genericHttp_resultInit(void) {

	protocol_register(&generic_http_result);
	protocol_set_id(generic_http_result, "generic_http_result");
	protocol_device_add(generic_http_result, "generic_http_result", "Generic http result");
	//generic_http_result->devtype = LABEL;

	options_add(&generic_http_result->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,})$");
	options_add(&generic_http_result->options, 'b', "busy", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&generic_http_result->options, 'r', "ready", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&generic_http_result->options, 'd', "data", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&generic_http_result->options, 's', "size", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&generic_http_result->options, 'c', "code", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_NUMBER, NULL, NULL);
	options_add(&generic_http_result->options, 'm', "mimetype", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	generic_http_result->printHelp=&printHelp;
	generic_http_result->createCode=&createCode;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "generic_http_result";
	module->version = "1.0";
	module->reqversion = "7.0";
	module->reqcommit = "84";
}

void init(void) {
	genericHttp_resultInit();
}
#endif
