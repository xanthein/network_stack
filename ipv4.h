#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>

struct ipv4_info_t {
	uint32_t ip;
};

struct ipv4_header {
	uint8_t ihl : 4;
	uint8_t version : 4;
	uint8_t ecn : 2;
	uint8_t dscp : 6;
	uint16_t length;
	uint16_t identification;
	uint16_t fragment_offset;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t checksum;
	uint32_t src_ip;
	uint32_t dst_ip;
	uint8_t data[];
} __attribute__((packed));

#define IP_ICMP	0x01
#define IP_TCP	0x06
#define IP_UDP	0x11

struct ipv4_info_t ipv4_info;

void ipv4_init();
int ipv4_read(uint8_t *buffer, uint32_t size);
void ipv4_write();

#endif
