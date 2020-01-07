#ifndef ICMPV4_H
#define ICMPV4_H

#include <stdint.h>

struct icmpv4_header {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint8_t data[];
} __attribute__((packed));

#define ICMPV4_HEADER_SIZE sizeof(struct icmpv4_header)

struct icmpv4_echo_header {
	uint16_t identifier;
	uint16_t sequence;
	uint8_t data[];
} __attribute__((packed));


#define ICMPV4_TYPE_REPLY	0x00
#define ICMPV4_TYPE_ECHO	0x08

int icmpv4_read(uint8_t *buffer, uint32_t size);
int icmpv4_write();

#endif
