#include <cerver/http/http.h>
#include <cerver/http/request.h>
#include <cerver/http/response.h>

#include "controllers/service.h"

// GET /api/pocket
void pocket_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (pocket_works, http_receive);

}

// GET /api/pocket/version
void pocket_version_handler (
	const HttpReceive *http_receive,
	const HttpRequest *request
) {

	(void) http_response_send (current_version, http_receive);

}