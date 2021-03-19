#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <signal.h>

#include <cerver/version.h>
#include <cerver/cerver.h>

#include <cerver/http/http.h>
#include <cerver/http/route.h>

#include <cerver/utils/utils.h>
#include <cerver/utils/log.h>

#include "handler.h"
#include "pocket.h"
#include "version.h"

#include "controllers/users.h"

#include "routes/categories.h"
#include "routes/places.h"
#include "routes/service.h"
#include "routes/transactions.h"
#include "routes/users.h"

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
	// GET /api/pocket
	HttpRoute *pocket_route = http_route_create (REQUEST_METHOD_GET, "api/pocket", pocket_handler);
	http_cerver_route_register (http_cerver, pocket_route);

	/* register pocket children routes */
	// GET api/pocket/version
	HttpRoute *pocket_version_route = http_route_create (REQUEST_METHOD_GET, "version", pocket_version_handler);
	http_route_child_add (pocket_route, pocket_version_route);

	// GET api/pocket/auth
	HttpRoute *pocket_auth_route = http_route_create (REQUEST_METHOD_GET, "auth", pocket_auth_handler);
	http_route_set_auth (pocket_auth_route, HTTP_ROUTE_AUTH_TYPE_BEARER);
	http_route_set_decode_data (pocket_auth_route, pocket_user_parse_from_json, pocket_user_delete);
	http_route_child_add (pocket_route, pocket_auth_route);

	/*** transactions ***/

	// GET api/pocket/transactions
	HttpRoute *transactions_route = http_route_create (REQUEST_METHOD_GET, "transactions", pocket_transactions_handler);
	http_route_set_auth (transactions_route, HTTP_ROUTE_AUTH_TYPE_BEARER);
	http_route_set_decode_data (transactions_route, pocket_user_parse_from_json, pocket_user_delete);
	http_route_child_add (pocket_route, transactions_route);

	// POST api/pocket/transactions
	http_route_set_handler (transactions_route, REQUEST_METHOD_POST, pocket_transaction_create_handler);

	// GET api/pocket/transactions/:id
	HttpRoute *single_trans_route = http_route_create (REQUEST_METHOD_GET, "transactions/:id", pocket_transaction_get_handler);
	http_route_set_auth (single_trans_route, HTTP_ROUTE_AUTH_TYPE_BEARER);
	http_route_set_decode_data (single_trans_route, pocket_user_parse_from_json, pocket_user_delete);
	http_route_child_add (pocket_route, single_trans_route);

	// PUT api/pocket/transactions/:id
	http_route_set_handler (single_trans_route, REQUEST_METHOD_PUT, pocket_transaction_update_handler);

	// DELETE api/pocket/transactions/:id
	http_route_set_handler (single_trans_route, REQUEST_METHOD_DELETE, pocket_transaction_delete_handler);

	/*** categories ***/

	// GET api/pocket/categories
	HttpRoute *categories_route = http_route_create (REQUEST_METHOD_GET, "categories", pocket_categories_handler);
	http_route_set_auth (categories_route, HTTP_ROUTE_AUTH_TYPE_BEARER);
	http_route_set_decode_data (categories_route, pocket_user_parse_from_json, pocket_user_delete);
	http_route_child_add (pocket_route, categories_route);

	// POST api/pocket/categories
	http_route_set_handler (categories_route, REQUEST_METHOD_POST, pocket_category_create_handler);

	// GET api/pocket/categories/:id
	HttpRoute *single_category_route = http_route_create (REQUEST_METHOD_GET, "categories/:id", pocket_category_get_handler);
	http_route_set_auth (single_category_route, HTTP_ROUTE_AUTH_TYPE_BEARER);
	http_route_set_decode_data (single_category_route, pocket_user_parse_from_json, pocket_user_delete);
	http_route_child_add (pocket_route, single_category_route);

	// PUT api/pocket/categories/:id
	http_route_set_handler (single_category_route, REQUEST_METHOD_PUT, pocket_category_update_handler);

	// DELETE api/pocket/categories/:id
	http_route_set_handler (single_category_route, REQUEST_METHOD_DELETE, pocket_category_delete_handler);

	/*** places ***/

	// GET api/pocket/places
	HttpRoute *places_route = http_route_create (REQUEST_METHOD_GET, "places", pocket_places_handler);
	http_route_set_auth (places_route, HTTP_ROUTE_AUTH_TYPE_BEARER);
	http_route_set_decode_data (places_route, pocket_user_parse_from_json, pocket_user_delete);
	http_route_child_add (pocket_route, places_route);

	// POST api/pocket/places
	http_route_set_handler (places_route, REQUEST_METHOD_POST, pocket_place_create_handler);

	// GET api/pocket/places/:id
	HttpRoute *single_place_route = http_route_create (REQUEST_METHOD_GET, "places/:id", pocket_place_get_handler);
	http_route_set_auth (single_place_route, HTTP_ROUTE_AUTH_TYPE_BEARER);
	http_route_set_decode_data (single_place_route, pocket_user_parse_from_json, pocket_user_delete);
	http_route_child_add (pocket_route, single_place_route);

	// PUT api/pocket/places/:id
	http_route_set_handler (single_place_route, REQUEST_METHOD_PUT, pocket_place_update_handler);

	// DELETE api/pocket/places/:id
	http_route_set_handler (single_place_route, REQUEST_METHOD_DELETE, pocket_place_delete_handler);

}

static void pocket_set_users_routes (HttpCerver *http_cerver) {

	/* register top level route */
	// GET /api/users
	HttpRoute *users_route = http_route_create (REQUEST_METHOD_GET, "api/users", users_handler);
	http_cerver_route_register (http_cerver, users_route);

	/* register users children routes */
	// POST api/users/login
	HttpRoute *users_login_route = http_route_create (REQUEST_METHOD_POST, "login", users_login_handler);
	http_route_child_add (users_route, users_login_route);

	// POST api/users/register
	HttpRoute *users_register_route = http_route_create (REQUEST_METHOD_POST, "register", users_register_handler);
	http_route_child_add (users_route, users_register_route);

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

		http_cerver_auth_set_jwt_algorithm (http_cerver, JWT_ALG_RS256);
		if (ENABLE_USERS_ROUTES) {
			http_cerver_auth_set_jwt_priv_key_filename (http_cerver, PRIV_KEY->str);
		}
		
		http_cerver_auth_set_jwt_pub_key_filename (http_cerver, PUB_KEY->str);

		pocket_set_pocket_routes (http_cerver);

		if (ENABLE_USERS_ROUTES) {
			pocket_set_users_routes (http_cerver);
		}

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