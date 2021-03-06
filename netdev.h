#ifndef NETDEV_H
#define NETDEV_H

#include <stdint.h>
#include <stdlib.h>

#define NETDEV_BUFFER_LEN 1500

uint8_t mac_address[6];

int netdev_open();
int netdev_close();
void netdev_read();
int netdev_write(uint8_t *buffer, size_t size, uint8_t *dst_mac, uint16_t type);

#endif
