#include <stdio.h>
#include <netinet/in.h>
#include "netdev.h"
#include "ethernet.h"
#include "ipv4.h"

uint16_t calculate_ipv4_checksum(uint16_t *header)
{
	uint32_t sum = 0;

	for(int i=0;i<10;i++)
		sum += header[i];

	return ~((sum>>16) + (sum&0xffff));
}

int ipv4_read(uint8_t *buffer, uint32_t size)
{
	struct ether_frame *ether = (struct ether_frame *)buffer;
	struct ipv4_header *header = (struct ipv4_header *)(ether->data);
	uint16_t checksum = calculate_ipv4_checksum((uint16_t *)header);

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
			break;
		case IP_TCP:
			printf("tcp\n");
			break;
		case IP_UDP:
			printf("udp\n");
			break;
		default:
			printf("ip protocol not support\n");
	}

	free(buffer);
	return 0;
}

void ipv4_init()
{
	ipv4_info.ip = 0xa000004;
}
