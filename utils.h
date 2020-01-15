#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

uint16_t calculate_checksum(uint8_t *header, uint32_t size);
uint16_t calculate_tcp_checksum(uint8_t *buffer, uint32_t size, uint32_t src_ip, uint32_t dst_ip, uint16_t length);
#endif
