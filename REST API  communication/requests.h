#ifndef _REQUESTS_
#define _REQUESTS_
#include "helpers.h"

// computes and returns a GET or DELETE request string (query_params
// and cookies can be set to NULL if not needed)
char *compute_request(string request_type, const char *host, const char *url,
				const char *query_params, char **cookies, int cookies_count,
				const char* authorization);

// computes and returns a POST request string (cookies can be NULL if not needed)
char *compute_post_request(const char *host, const char *url, const char* content_type,
				json body_data, int body_data_fields_count, char** cookies,
				int cookies_count, const char* authorization);

#endif
