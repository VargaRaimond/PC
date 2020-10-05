#include "requests.h"

char *compute_request(string request_type, const char *host, const char *url,
                const char *query_params, char **cookies, int cookies_count,
                const char* authorization) {
    char *message = (char*)calloc(BUFLEN, sizeof(char));
    char *line = (char*)calloc(LINELEN, sizeof(char));

    // write url, version and chosen type - get or delete
    if (query_params != NULL) {
        sprintf(line, "%s %s?%s HTTP/1.1", request_type.c_str(),
            url, query_params);
    } else {
        sprintf(line, "%s %s HTTP/1.1", request_type.c_str(), url);
    }
    compute_message(message, line);
    // add host
    memset(line, 0, LINELEN);
    sprintf(line, "HOST: %s", host);
    compute_message(message, line);
    if (authorization != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer ");
        strcat(line, authorization);

        compute_message(message, line);
    }
    // add additional headers and cookies
    if (cookies != NULL) {
       memset(line, 0, LINELEN);
       sprintf(line, "Cookie: ");

       for(int i = 0; i < cookies_count - 1; i++) {
        strcat(line, cookies[i]);
        strcat(line, "; ");
       }
       strcat(line, cookies[cookies_count-1]);
       compute_message(message, line);
    }
    compute_message(message, "");
    return message;
}

char *compute_post_request(const char *host, const char *url, const char* content_type,
                    json body_data, int body_data_fields_count, char **cookies,
                    int cookies_count, const char* authorization)
{
    char *message = (char*)calloc(BUFLEN, sizeof(char));
    char *line = (char*)calloc(LINELEN, sizeof(char));

    // first line includes url and type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // add host to message
    memset(line, 0, LINELEN);
    sprintf(line, "HOST: %s", host);
    compute_message(message, line);


    if (authorization != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer ");
        strcat(line, authorization);

        compute_message(message, line);
    }
    // add all necessary headers
    memset(line, 0, LINELEN);
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Content-Length: %lu", body_data.dump().size());
    compute_message(message, line);
    // add cookies if we have any
    if (cookies != NULL) {
        memset(line, 0, LINELEN);
        sprintf(line, "Cookie: ");
 
       for(int i = 0; i < cookies_count - 1; i++) {
        strcat(line, cookies[i]);
        strcat(line, ";");
       }
       strcat(line, cookies[cookies_count-1]);
       compute_message(message, line);
    }
    // finish message with newline
    compute_message(message, "");
    // add our data json
    memset(line, 0, LINELEN);
    compute_message(message, body_data.dump().c_str());

    free(line);
    return message;
}
