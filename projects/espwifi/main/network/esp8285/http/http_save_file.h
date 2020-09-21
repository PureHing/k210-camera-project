#ifndef __HTTP_SAVE_FILE_H
#define __HTTP_SAVE_FILE_H

#define DEBUG_HTTP_TIME 0

uint32_t http_save_file(uint8_t sock, char *resp_header, uint32_t resp_header_len, uint8_t *file, uint32_t file_len);

#endif