#ifndef __HTTP_SIMPLE_H
#define __HTTP_SIMPLE_H

/*
	Represents an HTTP html response
*/
struct http_response
{
    struct parsed_url *request_uri;
    char *body;
    uint32_t body_len;
    char *status_code;
    int status_code_int;
    char *status_text;
    char *request_headers;
    char *response_headers;
};

struct http_response *http_get(char *url, char *custom_headers);
struct http_response *http_post(char *url, char *custom_headers, char *post_data);

char* http_get_header_filed(char *header, char *filed);

void http_response_free(struct http_response *hresp);

#endif