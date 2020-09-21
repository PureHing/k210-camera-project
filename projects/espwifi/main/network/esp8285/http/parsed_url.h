#ifndef __PARSED_URL_H
#define __PARSED_URL_H

#define NOT_ONLY_SUPPORT_GET 0

struct parsed_url
{
    char *uri;    /* mandatory */
    char *scheme; /* mandatory */
    char *host;   /* mandatory */
    char *ip;     /* mandatory */
    char *port;   /* optional */
    char *path;   /* optional */
    char *query;  /* optional */
#if NOT_ONLY_SUPPORT_GET
    char *fragment; /* optional */
    char *username; /* optional */
    char *password; /* optional */
#endif
};

void parsed_url_free(struct parsed_url *purl);
struct parsed_url *parse_url(const char *url);

#endif