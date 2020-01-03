#ifndef ARP_H
#define ARP_H

#include <stdint.h>

#define ARP_ETHERNET    0x1

#define ARP_IPV4        0x0800

#define ARP_REQUEST     0x1
#define ARP_REPLY       0x2

struct arp_header {
	uint16_t hardware_type;
	uint16_t protocol_type;
	uint8_t hardware_len;
	uint8_t protocol_len;
	uint16_t operation;
	uint8_t sender_mac[6];
	uint32_t sender_ip;
	uint8_t target_mac[6];
	uint32_t target_ip;
} __attribute__((packed));

#define ARP_HEADER_SIZE sizeof(struct arp_header)

struct arp_trans_table {
	uint8_t mac[6];
	uint32_t ip;
	uint16_t protocol;
	uint8_t age;
};

int arp_read(uint8_t *buffer, uint32_t size);
void arp_request(uint32_t target_ip);
int arp_init();
int arp_close();

#endif
