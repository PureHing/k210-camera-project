#ifndef __MYSPI_H
#define __MYSPI_H

#include <stdint.h>

void my_spi_cs_set(void);
void my_spi_cs_clr(void);

void my_spi_init(void);
uint8_t my_spi_rw(uint8_t data);
void my_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len);
uint64_t get_millis(void);

#endif

