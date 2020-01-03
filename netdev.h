#ifndef NETDEV_H
#define NETDEV_H

#include <stdint.h>

#define NETDEV_BUFFER_LEN 1500

int netdev_open();
int netdev_close();
void netdev_read();
void netdev_write(uint8_t *buffer, size_t size);

#endif