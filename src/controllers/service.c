#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cerver/http/response.h>

#include <cerver/utils/utils.h>

#include "version.h"

HttpResponse *pocket_works = NULL;
HttpResponse *current_version = NULL;

HttpResponse *catch_all = NULL;

unsigned int pocket_service_init (void) {

	unsigned int retval = 1;

	pocket_works = http_response_json_key_value (
		HTTP_STATUS_OK, "msg", "Pocket status works!"
	);

	char *status = c_string_create (
		"%s - %s",
		POCKET_VERSION_NAME, POCKET_VERSION_DATE
	);

	if (status) {
		current_version = http_response_json_key_value (
			HTTP_STATUS_OK, "version", status
		);

		free (status);
	}

	catch_all = http_response_json_key_value (
		HTTP_STATUS_OK, "msg", "Pocket status service!"
	);

	if (
		catch_all
		&& pocket_works && current_version
	) retval = 0;

	return retval;

}

void pocket_service_end (void) {

	http_response_delete (pocket_works);
	http_response_delete (current_version);

	http_response_delete (catch_all);

}
