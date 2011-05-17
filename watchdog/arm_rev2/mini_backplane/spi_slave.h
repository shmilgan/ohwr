#ifndef __SPI_SLAVE_H
#define __SPI_SLAVE_H

int spi_slave_poll();
int spi_slave_read(unsigned char *buf, int max_len);
int spi_slave_write(unsigned char *buf, int len);
void spi_slave_init();

#endif