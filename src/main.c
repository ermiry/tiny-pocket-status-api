#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <signal.h>

#include <cerver/version.h>
#include <cerver/cerver.h>

#include <cerver/http/http.h>
#include <cerver/http/route.h>

#include <cerver/utils/log.h>
#include <cerver/utils/utils.h>

#include "pocket.h"
#include "version.h"

#include "routes/service.h"

static Cerver *pocket_api = NULL;

void end (int dummy) {
	
	if (pocket_api) {
		cerver_stats_print (pocket_api, false, false);
		cerver_log_msg ("\nHTTP Cerver stats:\n");
		http_cerver_all_stats_print ((HttpCerver *) pocket_api->cerver_data);
		cerver_log_line_break ();
		cerver_teardown (pocket_api);
	}

	(void) pocket_end ();

	cerver_end ();

	exit (0);

}

static void pocket_set_pocket_routes (HttpCerver *http_cerver) {

	/* register top level route */
	// GET /api/status
	HttpRoute *pocket_route = http_route_create (REQUEST_METHOD_GET, "api/status", pocket_handler);
	http_cerver_route_register (http_cerver, pocket_route);

	/* register pocket children routes */
	// GET api/status/version
	HttpRoute *pocket_version_route = http_route_create (REQUEST_METHOD_GET, "version", pocket_version_handler);
	http_route_child_add (pocket_route, pocket_version_route);

}

static void start (void) {

	pocket_api = cerver_create (
		CERVER_TYPE_WEB,
		"pocket-api",
		PORT,
		PROTOCOL_TCP,
		false,
		CERVER_CONNECTION_QUEUE
	);

	if (pocket_api) {
		/*** cerver configuration ***/
		cerver_set_receive_buffer_size (pocket_api, CERVER_RECEIVE_BUFFER_SIZE);
		cerver_set_thpool_n_threads (pocket_api, CERVER_TH_THREADS);
		cerver_set_handler_type (pocket_api, CERVER_HANDLER_TYPE_THREADS);

		/*** web cerver configuration ***/
		HttpCerver *http_cerver = (HttpCerver *) pocket_api->cerver_data;

		pocket_set_pocket_routes (http_cerver);

		// add a catch all route
		http_cerver_set_catch_all_route (http_cerver, pocket_catch_all_handler);

		if (cerver_start (pocket_api)) {
			cerver_log_error (
				"Failed to start %s!",
				pocket_api->info->name->str
			);

			cerver_delete (pocket_api);
		}
	}

	else {
		cerver_log_error ("Failed to create cerver!");

		cerver_delete (pocket_api);
	}

}

int main (int argc, char const **argv) {

	srand (time (NULL));

	// register to the quit signal
	(void) signal (SIGINT, end);
	(void) signal (SIGTERM, end);

	// to prevent SIGPIPE when writting to socket
	(void) signal (SIGPIPE, SIG_IGN);

	cerver_init ();

	cerver_version_print_full ();

	pocket_version_print_full ();

	if (!pocket_init ()) {
		start ();
	}

	else {
		cerver_log_error ("Failed to init pocket!");
	}

	(void) pocket_end ();

	cerver_end ();

	return 0;

}