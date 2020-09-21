#ifndef __HTTP_GET_FILE_H
#define __HTTP_GET_FILE_H

uint32_t http_get_file(char *url,
                       char *custom_headers,
                       char *resp_header,
                       uint32_t resp_header_len,
                       uint8_t *file,
                       uint32_t file_len);

uint32_t http_post_file(char *url,
                        char *custom_headers,
                        uint8_t *body,
                        uint8_t *boundary,
                        uint8_t *post_file,
                        uint32_t post_file_len,
                        char *resp_header,
                        uint32_t resp_header_len,
                        uint8_t *file,
                        uint32_t file_len);

#endif
