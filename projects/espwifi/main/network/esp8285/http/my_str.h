#ifndef __MY_STR_H
#define __MY_STR_H

// int my_atoi(char *string);

int my_isalpha(int ch);
char my_isdigit(unsigned char c);
int my_isalnum(int c);
int my_str_index_of(const char *a, char *b);
int my_str_contains(const char *haystack, const char *needle);
char *my_trim_end(char *string, char to_trim);
char *my_str_cat(char *a, char *b);
char my_to_hex(char code);
char *my_urlencode(char *str);
char *my_str_ndup(const char *str, size_t max);
char *my_str_dup(const char *src);
char *my_str_replace(char *search, char *replace, char *subject);
char *my_get_until(char *haystack, char *until);
void my_decodeblock(unsigned char in[], char *clrstr);
char *my_base64_decode(char *b64src);
void my_encodeblock(unsigned char in[], char b64str[], int len);
char *my_base64_encode(char *clrstr);

#endif
