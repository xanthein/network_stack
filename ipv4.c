#include <stdio.h>
#include <netinet/in.h>
#include "netdev.h"
#include "ethernet.h"
#include "arp.h"
#include "ipv4.h"
#include "icmpv4.h"
#include "utils.h"

int ipv4_read(uint8_t *buffer, uint32_t size)
{
	struct ether_frame *ether = (struct ether_frame *)buffer;
	struct ipv4_header *header = (struct ipv4_header *)(ether->data);
	uint16_t checksum = calculate_checksum((uint8_t *)header, IPV4_HEADER_SIZE);

	if(checksum) {
		printf("ipv4 checksum error\n");
		free(buffer);
		return -1;
	}

	if(header->version != 4) {
		printf("version not match\n");
		free(buffer);
		return -1;
	}

	if(header->ihl < 5) {
		printf("header length too short\n");
		free(buffer);
		return -1;
	}

	//TODO need to handle fragmentation

	header->length = ntohs(header->length);
	header->identification = ntohs(header->identification);
	header->src_ip = ntohl(header->src_ip);
	header->dst_ip = ntohl(header->dst_ip);

	switch(header->protocol) {
		case IP_ICMP:
			printf("icmp\n");
			free(buffer);
			break;
		case IP_TCP:
			printf("tcp\n");
			free(buffer);
			break;
		case IP_UDP:
			printf("udp\n");
			free(buffer);
			break;
		default:
			printf("ip protocol not support\n");
			free(buffer);
			break;
	}

	return 0;
}

int ipv4_write(uint8_t *buffer, uint32_t size, uint32_t dst_ip, uint8_t protocol)
{
	struct ether_frame *ether = (struct ether_frame *)buffer;
	struct ipv4_header *header = (struct ipv4_header *)(ether->data);
	struct arp_trans_table *table;

	header->ihl = 5;
	header->version = 4;
	header->length = htons(size - ETH_HEADER_SIZE);
	header->fragment_offset = htons(IP_FLAGS_DF);
	header->ttl = 64;
	header->protocol = protocol;
	header->src_ip = htonl(ipv4_info.ip);
	header->dst_ip = htonl(dst_ip);
	header->checksum = calculate_checksum((uint8_t *)header, IPV4_HEADER_SIZE);

	table = trans_table_lookup(dst_ip);
	if(table != NULL)
		return netdev_write(buffer, size, table->mac, ETH_TYPE_IP);
	else {
		arp_request(dst_ip);
		return -1;
	}
}

void ipv4_init()
{
	ipv4_info.ip = 0xa000004;
}
