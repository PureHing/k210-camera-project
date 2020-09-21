/*
	http-client-c
	Copyright (C) 2012-2013  Swen Kooij

	This file is part of http-client-c.

    http-client-c is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    http-client-c is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with http-client-c. If not, see <http://www.gnu.org/licenses/>.

	Warning:
	This library does not tend to work that stable nor does it fully implent the
	standards described by IETF. For more information on the precise implentation of the
	Hyper Text Transfer Protocol:

	http://www.ietf.org/rfc/rfc2616.txt
*/

#pragma GCC diagnostic ignored "-Wwrite-strings"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "http_simple.h"
#include "my_str.h"
#include "parsed_url.h"

#include "sleep.h"
#include "WiFiSpiClient.h"

#define HTTP_BUF_SIZE 1024 * 150

static struct http_response *http_head(char *url, char *custom_headers);

/*
	Handles redirect if needed for get requests
*/
static struct http_response *handle_redirect_get(struct http_response *hresp, char *custom_headers)
{
	if (hresp->status_code_int > 300 && hresp->status_code_int < 399)
	{
		char *token = strtok(hresp->response_headers, "\r\n");
		while (token != NULL)
		{
			if (my_str_contains(token, "Location:"))
			{
				/* Extract url */
				char *location = my_str_replace("Location: ", "", token);
				return http_get(location, custom_headers);
			}
			token = strtok(NULL, "\r\n");
		}
	}
	else
	{
		/* We're not dealing with a redirect, just return the same structure */
		return hresp;
	}
	return NULL;
}

/*
	We don't need this func
	Handles redirect if needed for head requests
*/
static struct http_response *handle_redirect_head(struct http_response *hresp, char *custom_headers)
{
	if (hresp->status_code_int > 300 && hresp->status_code_int < 399)
	{
		char *token = strtok(hresp->response_headers, "\r\n");
		while (token != NULL)
		{
			if (my_str_contains(token, "Location:"))
			{
				/* Extract url */
				char *location = my_str_replace("Location: ", "", token);
				return http_head(location, custom_headers);
			}
			token = strtok(NULL, "\r\n");
		}
	}
	else
	{
		/* We're not dealing with a redirect, just return the same structure */
		return hresp;
	}
}

/*
	Handles redirect if needed for post requests
*/
static struct http_response *handle_redirect_post(struct http_response *hresp, char *custom_headers, char *post_data)
{
	if (hresp->status_code_int > 300 && hresp->status_code_int < 399)
	{
		char *token = strtok(hresp->response_headers, "\r\n");
		while (token != NULL)
		{
			if (my_str_contains(token, "Location:"))
			{
				/* Extract url */
				char *location = my_str_replace("Location: ", "", token);
				return http_post(location, custom_headers, post_data);
			}
			token = strtok(NULL, "\r\n");
		}
	}
	else
	{
		/* We're not dealing with a redirect, just return the same structure */
		return hresp;
	}
	return NULL;
}

/*
	Makes a HTTP request and returns the response
*/
static struct http_response *http_req(char *http_headers, struct parsed_url *purl)
{
	/* Parse url */
	if (purl == NULL)
	{
		printk("Unable to parse url");
		return NULL;
	}

	/* Declare variable */
	// int sock;
	// int tmpres;
	char buf[BUFSIZ + 1];
	// struct sockaddr_in *remote;
	uint32_t tmp_len, tmp;

	/* Allocate memeory for htmlcontent */
	struct http_response *hresp = (struct http_response *)malloc(sizeof(struct http_response));
	if (hresp == NULL)
	{
		printk("Unable to allocate memory for htmlcontent.");
		return NULL;
	}

	hresp->body = NULL;
	hresp->request_headers = NULL;
	hresp->response_headers = NULL;
	hresp->status_code = NULL;
	hresp->status_text = NULL;

#if 0
	/* Create TCP socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printk("Can't create TCP socket");
		return NULL;
	}

	/* Set remote->sin_addr.s_addr */
	remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
	remote->sin_family = AF_INET;
	tmpres = inet_pton(AF_INET, purl->ip, (void *)(&(remote->sin_addr.s_addr)));
	if (tmpres < 0)
	{
		printk("Can't set remote->sin_addr.s_addr");
		return NULL;
	}
	else if (tmpres == 0)
	{
		printk("Not a valid IP");
		return NULL;
	}
	remote->sin_port = htons(atoi(purl->port));

	/* Connect */
	if (connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0)
	{
		printk("Could not connect");
		return NULL;
	}
#else
	uint8_t sock = WiFiSpiClient_connect_host(purl->host, atoi(purl->port),0);
	if (sock != SOCK_NOT_AVAIL)
	{
		printk("connect to server ok\r\n");
	}
	else
	{
		printk("connect to server failed\r\n");
		return NULL;
	}
#endif
#if 0
	/* Send headers to server */
	int sent = 0;
	while (sent < strlen(http_headers))
	{
		tmpres = send(sock, http_headers + sent, strlen(http_headers) - sent, 0);
		if (tmpres == -1)
		{
			printk("Can't send headers");
			return NULL;
		}
		sent += tmpres;
	}
#else
	/* Send headers to server */
	int sent = 0;
	tmp_len = strlen(http_headers);

	while (sent < tmp_len)
	{
		if ((tmp_len - sent) > 1024)
		{
			tmp = 1024;
			if (WiFiSpiClient_write(sock, http_headers + sent, tmp) != tmp)
			{
				printk("Send header to server failed\n");
				return NULL;
			}
		}
		else
		{
			tmp = tmp_len - sent;
			if (WiFiSpiClient_write(sock, http_headers + sent, tmp) != tmp)
			{
				printk("Send header to server failed\n");
				return NULL;
			}
		}
		sent += tmp;
	}
#endif

	/* Recieve into response*/
	// char *response = (char *)malloc(0);
	char *response = (char *)malloc(HTTP_BUF_SIZE);
	char BUF[BUFSIZ];
	size_t recived_len = 0;
	uint8_t poll_times, flag = 1;
	uint32_t read_len, buff_len, last_buf_len = 9999;

#if 0
	while ((recived_len = recv(sock, BUF, BUFSIZ - 1, 0)) > 0)
	{
		BUF[recived_len] = '\0';
		response = (char *)realloc(response, strlen(response) + strlen(BUF) + 1);
		sprintf(response, "%s%s", response, BUF);
	}
#else
	while (flag)
	{
		memset(BUF, 0, BUFSIZ);
		poll_times = 250;
		do
		{
			buff_len = WiFiSpiClient_available(sock);
#if 0
            if ((buff_len > 1024) || (last_buf_len == buff_len))
            {
                break;
            }
            if (buff_len > 0)
                last_buf_len = buff_len;
#else
			if (buff_len > 0)
				break;
#endif
			msleep(2); //5ms*100=500ms
		} while (--poll_times > 0);

		if (buff_len == 0)
		{
			/* receive error or no data*/
			flag = 0;
			printk("%s\r\n", recived_len ? "receive over" : "receive failed");
		}
		else
		{
			if (buff_len > BUFSIZ)
			{
				read_len = BUFSIZ;
				WiFiSpiClient_read_buf(sock, BUF, read_len);
			}
			else
			{
				read_len = buff_len;
				WiFiSpiClient_read_buf(sock, BUF, read_len);
			}
			memcpy(response + recived_len, BUF, read_len);
			recived_len += read_len;
		}
	}
#endif

	WiFiSpiClient_stop(sock);

	if (recived_len < 0)
	{
		free(http_headers);
		printk("Unabel to recieve");
		return NULL;
	}

	/* Reallocate response */
	// response = (char *)realloc(response, strlen(response) + 1);
	response = (char *)realloc(response, recived_len + 1);
	response[recived_len] = 0;

	// printk("\r\nrecived_len:%ld\r\n%s\r\n\r\n\r\n", recived_len, response);

#if 0
	/* Parse status code and text */
	char *status_line = my_get_until(response, "\r\n");
	status_line = my_str_replace("HTTP/1.1 ", "", status_line);
	printk("status_line:%s\r\n", status_line);

	char *status_code = my_str_ndup(status_line, 4);
	status_code = my_str_replace(" ", "", status_code);
	printk("status_code:%s\r\n", status_code);

	char *status_text = my_str_replace(status_code, "", status_line);
	status_text = my_str_replace(" ", "", status_text);
	printk("status_text:%s\r\n", status_text);
#else
	/* Parse status code and text */
	char *p1 = NULL;
	uint32_t str_len1 = 0, str_len2 = 0;
	char tmp1[50], tmp2[50];
	char *status_code = NULL;
	char *status_text = NULL;

	//dirty code
	p1 = strstr(response, "\r\n");
	if (p1 != NULL)
	{
		str_len1 = p1 - response;
		memcpy(tmp1, response, str_len1);
		// printk("tmp1:%s\tstr_len1:%d\r\n", tmp1, str_len1); //HTTP/1.1 200 OK
		p1 = strstr(tmp1, "HTTP/1.");
		if (p1 != NULL)
		{
			str_len2 = p1 - tmp1;
			memcpy(tmp2, p1 + 9, str_len1 - str_len2 - 9);
			// printk("tmp2:%s\tlen:%d\r\n", tmp2, str_len1 - str_len2 - 9);
			status_code = (char *)malloc(4);
			memcpy(status_code, tmp2, 3);
			status_code[3] = 0;

			status_text = (char *)malloc((str_len1 - str_len2 - 13) + 1);
			memcpy(status_text, p1 + 13, (str_len1 - str_len2 - 13));
			status_text[(str_len1 - str_len2 - 13)] = 0;
		}
		else
		{
			printk("error @ %d\r\n", __LINE__);
		}
	}
	else
	{
		printk("error @ %d\r\n", __LINE__);
	}
#endif
	hresp->status_code = status_code;
	hresp->status_code_int = atoi(status_code);
	hresp->status_text = status_text;

	/* Parse response headers */
	char *headers = my_get_until(response, "\r\n\r\n");
	hresp->response_headers = headers;

	/* Assign request headers */
	hresp->request_headers = http_headers;

	/* Assign request url */
	hresp->request_uri = purl;

	/* Parse body */
	char *body = strstr(response, "\r\n\r\n");
	char *body_tmp = body + 4;

	body = my_str_replace("\r\n\r\n", "", body);
	hresp->body = body;

	// printk("head_len:%ld\r\n\r\n", body_tmp - response);
	hresp->body_len = recived_len - (body_tmp - response);

	/* Return response */
	return hresp;
}

/*
	Makes a HTTP GET request to the given url
*/
struct http_response *http_get(char *url, char *custom_headers)
{
	/* Parse url */
	struct parsed_url *purl = parse_url(url);
	if (purl == NULL)
	{
		printk("Unable to parse url");
		return NULL;
	}

	/* Declare variable */
	char *http_headers = (char *)malloc(1024);
	char *tmp = (char *)malloc(1024);

	/* Build query/headers */
	if (purl->path != NULL)
	{
		if (purl->query != NULL)
		{
			sprintf(http_headers, "GET /%s?%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n", purl->path, purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "GET /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n", purl->path, purl->host);
		}
	}
	else
	{
		if (purl->query != NULL)
		{
			sprintf(http_headers, "GET /?%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n", purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "GET / HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n", purl->host);
		}
	}
#if NOT_ONLY_SUPPORT_GET
	/* Handle authorisation if needed */
	if (purl->username != NULL)
	{
		/* Format username:password pair */
		char *upwd = (char *)malloc(1024);
		sprintf(upwd, "%s:%s", purl->username, purl->password);
		upwd = (char *)realloc(upwd, strlen(upwd) + 1);

		/* Base64 encode */
		char *base64 = base64_encode(upwd);

		/* Form header */
		char *auth_header = (char *)malloc(1024);
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		auth_header = (char *)realloc(auth_header, strlen(auth_header) + 1);

		/* Add to header */
		http_headers = (char *)realloc(http_headers, strlen(http_headers) + strlen(auth_header) + 2);
		sprintf(http_headers, "%s%s", http_headers, auth_header);
	}
#endif
	/* Add custom headers, and close */
	if (custom_headers != NULL)
	{
		sprintf(tmp, "%s%s\r\n", http_headers, custom_headers);
	}
	else
	{
		sprintf(tmp, "%s\r\n", http_headers);
	}
	free(http_headers);
	tmp = (char *)realloc(tmp, strlen(tmp) + 1);

	/* Make request and return response */
	struct http_response *hresp = http_req(tmp, purl);

	/* Handle redirect */
	return handle_redirect_get(hresp, custom_headers);
}

/*
	Makes a HTTP POST request to the given url
*/
struct http_response *http_post(char *url, char *custom_headers, char *post_data)
{
	/* Parse url */
	struct parsed_url *purl = parse_url(url);
	if (purl == NULL)
	{
		printk("Unable to parse url");
		return NULL;
	}

	/* Declare variable */
	char *http_headers = (char *)malloc(1024);
	char *tmp = (char *)malloc(1024);

	/* Build query/headers */
	if (purl->path != NULL)
	{
		if (purl->query != NULL)
		{
			sprintf(http_headers, "POST /%s?%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nContent-Length: %zu\r\nContent-Type: application/x-www-form-urlencoded\r\n", purl->path, purl->query, purl->host, strlen(post_data));
		}
		else
		{
			sprintf(http_headers, "POST /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nContent-Length: %zu\r\nContent-Type: application/x-www-form-urlencoded\r\n", purl->path, purl->host, strlen(post_data));
		}
	}
	else
	{
		if (purl->query != NULL)
		{
			sprintf(http_headers, "POST /?%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nContent-Length: %zu\r\nContent-Type: application/x-www-form-urlencoded\r\n", purl->query, purl->host, strlen(post_data));
		}
		else
		{
			sprintf(http_headers, "POST / HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nContent-Length: %zu\r\nContent-Type: application/x-www-form-urlencoded\r\n", purl->host, strlen(post_data));
		}
	}
#if NOT_ONLY_SUPPORT_GET
	/* Handle authorisation if needed */
	if (purl->username != NULL)
	{
		/* Format username:password pair */
		char *upwd = (char *)malloc(1024);
		sprintf(upwd, "%s:%s", purl->username, purl->password);
		upwd = (char *)realloc(upwd, strlen(upwd) + 1);

		/* Base64 encode */
		char *base64 = base64_encode(upwd);

		/* Form header */
		char *auth_header = (char *)malloc(1024);
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		auth_header = (char *)realloc(auth_header, strlen(auth_header) + 1);

		/* Add to header */
		http_headers = (char *)realloc(http_headers, strlen(http_headers) + strlen(auth_header) + 2);
		sprintf(http_headers, "%s%s", http_headers, auth_header);
	}
#endif
	if (custom_headers != NULL)
	{
		sprintf(tmp, "%s%s\r\n", http_headers, custom_headers);
		sprintf(tmp, "%s\r\n%s", http_headers, post_data);
	}
	else
	{
		sprintf(tmp, "%s\r\n%s", http_headers, post_data);
	}
	tmp = (char *)realloc(tmp, strlen(tmp) + 1);

	/* Make request and return response */
	struct http_response *hresp = http_req(tmp, purl);

	/* Handle redirect */
	return handle_redirect_post(hresp, custom_headers, post_data);
}

/*
	We don't need this func
	Makes a HTTP HEAD request to the given url
*/
static struct http_response *http_head(char *url, char *custom_headers)
{
	/* Parse url */
	struct parsed_url *purl = parse_url(url);
	if (purl == NULL)
	{
		printk("Unable to parse url");
		return NULL;
	}

	/* Declare variable */
	char *http_headers = (char *)malloc(1024);
	char *tmp = (char *)malloc(1024);

	/* Build query/headers */
	if (purl->path != NULL)
	{
		if (purl->query != NULL)
		{
			sprintf(http_headers, "HEAD /%s?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->path, purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "HEAD /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->path, purl->host);
		}
	}
	else
	{
		if (purl->query != NULL)
		{
			sprintf(http_headers, "HEAD/?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "HEAD / HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->host);
		}
	}
#if NOT_ONLY_SUPPORT_GET
	/* Handle authorisation if needed */
	if (purl->username != NULL)
	{
		/* Format username:password pair */
		char *upwd = (char *)malloc(1024);
		sprintf(upwd, "%s:%s", purl->username, purl->password);
		upwd = (char *)realloc(upwd, strlen(upwd) + 1);

		/* Base64 encode */
		char *base64 = base64_encode(upwd);

		/* Form header */
		char *auth_header = (char *)malloc(1024);
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		auth_header = (char *)realloc(auth_header, strlen(auth_header) + 1);

		/* Add to header */
		http_headers = (char *)realloc(http_headers, strlen(http_headers) + strlen(auth_header) + 2);
		sprintf(http_headers, "%s%s", http_headers, auth_header);
	}
#endif
	if (custom_headers != NULL)
	{
		sprintf(tmp, "%s%s\r\n", http_headers, custom_headers);
	}
	else
	{
		sprintf(tmp, "%s\r\n", http_headers);
	}
	tmp = (char *)realloc(tmp, strlen(tmp) + 1);

	/* Make request and return response */
	struct http_response *hresp = http_req(tmp, purl);

	/* Handle redirect */
	return handle_redirect_head(hresp, custom_headers);
}

/*
	We don't need this func
	Do HTTP OPTIONs requests
*/
static struct http_response *http_options(char *url)
{
	/* Parse url */
	struct parsed_url *purl = parse_url(url);
	if (purl == NULL)
	{
		printk("Unable to parse url");
		return NULL;
	}

	/* Declare variable */
	char *http_headers = (char *)malloc(1024);
	char *tmp = (char *)malloc(1024);

	/* Build query/headers */
	if (purl->path != NULL)
	{
		if (purl->query != NULL)
		{
			sprintf(http_headers, "OPTIONS /%s?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->path, purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "OPTIONS /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->path, purl->host);
		}
	}
	else
	{
		if (purl->query != NULL)
		{
			sprintf(http_headers, "OPTIONS/?%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->query, purl->host);
		}
		else
		{
			sprintf(http_headers, "OPTIONS / HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n", purl->host);
		}
	}
#if NOT_ONLY_SUPPORT_GET
	/* Handle authorisation if needed */
	if (purl->username != NULL)
	{
		/* Format username:password pair */
		char *upwd = (char *)malloc(1024);
		sprintf(upwd, "%s:%s", purl->username, purl->password);
		upwd = (char *)realloc(upwd, strlen(upwd) + 1);

		/* Base64 encode */
		char *base64 = base64_encode(upwd);

		/* Form header */
		char *auth_header = (char *)malloc(1024);
		sprintf(auth_header, "Authorization: Basic %s\r\n", base64);
		auth_header = (char *)realloc(auth_header, strlen(auth_header) + 1);

		/* Add to header */
		http_headers = (char *)realloc(http_headers, strlen(http_headers) + strlen(auth_header) + 2);
		sprintf(http_headers, "%s%s", http_headers, auth_header);
	}
#endif
	/* Build headers */
	sprintf(tmp, "%s\r\n", http_headers);
	tmp = (char *)realloc(tmp, strlen(tmp) + 1);

	/* Make request and return response */
	struct http_response *hresp = http_req(tmp, purl);

	/* Handle redirect */
	return hresp;
}

/*
	from http response header get filed contents and return contents
	* filed must have `: `
*/
char *http_get_header_filed(char *header, char *filed)
{
	if (header == NULL || filed == NULL)
	{
		return 0;
	}

	char *contents = NULL, *p1 = NULL, *p2 = NULL;
	uint32_t len, clen = strlen(filed);

	if ((p1 = strstr(header, filed)) != NULL)
	{
		if ((p2 = strstr(p1, "\r\n")) != NULL)
		{
			len = p2 - p1 - clen;
			contents = (char *)malloc(sizeof(char) * len + 1);
			if (contents == NULL)
				return NULL;
			memcpy(contents, p1 + clen, len);
			contents[len] = '\0';
			return contents;
		}
	}
	return NULL;
}

/*
	Free memory of http_response
*/
void http_response_free(struct http_response *hresp)
{
	if (hresp != NULL)
	{
		if (hresp->request_uri != NULL)
			parsed_url_free(hresp->request_uri);
		if (hresp->body != NULL)
			free(hresp->body);
		if (hresp->status_code != NULL)
			free(hresp->status_code);
		if (hresp->status_text != NULL)
			free(hresp->status_text);
		if (hresp->request_headers != NULL)
			free(hresp->request_headers);
		if (hresp->response_headers != NULL)
			free(hresp->response_headers);
		free(hresp);
	}
}
