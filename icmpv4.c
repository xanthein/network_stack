#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "netdev.h"
#include "ethernet.h"
#include "ipv4.h"
#include "icmpv4.h"
#include "utils.h"

int icmpv4_reply(uint8_t*buffer, uint32_t size)
{
	struct ether_frame *ether = (struct ether_frame *)buffer;
	struct ipv4_header *ipv4 = (struct ipv4_header *)(ether->data);
	struct icmpv4_header *header = (struct icmpv4_header *)(ipv4->data);
	//struct icmpv4_echo_header *echo = (struct 

	uint32_t dst_ip = ipv4->src_ip;

	memset(ipv4, 0, IPV4_HEADER_SIZE);
	memset(header, 0, ICMPV4_HEADER_SIZE);
	
	header->type = ICMPV4_TYPE_REPLY;
	header->checksum = calculate_checksum((uint8_t *)header, 
			size - ETH_HEADER_SIZE - IPV4_HEADER_SIZE);

	return ipv4_write(buffer, size, dst_ip, IP_ICMP);
}

int icmpv4_read(uint8_t *buffer, uint32_t size)
{
	struct ether_frame *ether = (struct ether_frame *)buffer;
	struct ipv4_header *ipv4 = (struct ipv4_header *)(ether->data);
	struct icmpv4_header *header = (struct icmpv4_header *)(ipv4->data);

	switch(header->type) {
		case ICMPV4_TYPE_ECHO:
			if(icmpv4_reply(buffer, size) < 0)
				printf("icmpv4 reply failed\n");
			break;
		defalt:
			printf("icmp type %d not support\n", header->type);
			break;
	}
	
	free(buffer);
	return 0;
}

