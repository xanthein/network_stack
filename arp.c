#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "netdev.h"
#include "ethernet.h"
#include "arp.h"
#include "ipv4.h"

#define TRANS_TABLE_SIZE 128
struct arp_trans_table tables[TRANS_TABLE_SIZE];

uint8_t mac_address[6];

struct arp_trans_table* trans_table_lookup(uint32_t ip)
{
	for(int i=0;i<TRANS_TABLE_SIZE;i++) {
		if(ip == tables[i].ip)
			return &tables[i];
	}

	return NULL;
}

void trans_table_insert(uint32_t ip, uint8_t *mac, uint16_t protocol)
{
	uint8_t age = 0;
	struct arp_trans_table *table = NULL;
	
	for(int i=0;i<TRANS_TABLE_SIZE;i++) {
		if(tables[i].age >= age) {
			age = tables[i].age;
			table = &tables[i];
		}
	}

	if(table) {
		table->ip = ip;
		memcpy(&table->mac[0], mac, 6);
		table->protocol = protocol;
	}
}

//TODO need to call aging every 10 sec
void trans_table_aging(void)
{
	for(int i=0;i<TRANS_TABLE_SIZE;i++) {
		if(tables[i].ip != 0)
			tables[i].age++;
	}
}

int arp_read(uint8_t *buffer, uint32_t size)
{
	struct ether_frame *ether = (struct ether_frame *)buffer;
	struct arp_header *header = (struct arp_header *)(&ether->data[0]);

	header->hardware_type = ntohs(header->hardware_type);
	header->protocol_type = ntohs(header->protocol_type);
	header->operation = ntohs(header->operation);
	header->sender_ip = ntohl(header->sender_ip);
	header->target_ip = ntohl(header->target_ip);
    
	if(header->hardware_type != ARP_ETHERNET) {
		printf("ARP hardware type not support\n");
		free(buffer);
		return -1;
	}

	if(header->protocol_type != ARP_IPV4) {
		printf("ARP protocol type not support\n");
		free(buffer);
		return -1;
	}

	//check if ip already in the table
	//TODO

	//update translate table
	trans_table_insert(header->sender_ip, &header->sender_mac[0], header->protocol_type);

	//check if arp packet is for us
	if(ipv4_info.ip == header->target_ip) {
		switch(header->operation) {
			case ARP_REQUEST:
				memcpy(ether->dst_mac, ether->src_mac, 6);
				memcpy(ether->src_mac, mac_address, 6);
	
				header->hardware_type = htons(header->hardware_type);
				header->protocol_type = htons(header->protocol_type);
				header->operation = htons(ARP_REPLY);
				header->target_ip = ntohl(header->sender_ip);
				memcpy(header->target_mac, header->sender_mac, 6);
				header->sender_ip = ntohl(ipv4_info.ip);
				memcpy(header->sender_mac, mac_address, 6);

				netdev_write(buffer, size);

				break;
			default:
				printf("ARP operation not supported\n");
				break;
		}
	}
	
	free(buffer);
	return 0;
}

int arp_request()
{
}

int arp_init()
{
	mac_address[0] = 0x00;
	mac_address[1] = 0x0c;
	mac_address[2] = 0x29;
	mac_address[3] = 0x4c;
	mac_address[4] = 0x38;
	mac_address[5] = 0x22;
}

int arp_close()
{
}
