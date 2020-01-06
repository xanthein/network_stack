#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "netdev.h"
#include "ethernet.h"
#include "arp.h"
#include "ipv4.h"

#define TRANS_TABLE_SIZE 128
struct arp_trans_table arp_tables[TRANS_TABLE_SIZE];

uint8_t BROADCAST_MAC[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct arp_trans_table* trans_table_lookup(uint32_t ip)
{
	for(int i=0;i<TRANS_TABLE_SIZE;i++) {
		if(ip == arp_tables[i].ip)
			return &arp_tables[i];
	}

	return NULL;
}

void trans_table_insert(uint32_t ip, uint8_t *mac, uint16_t protocol)
{
	uint8_t age = 0;
	struct arp_trans_table *table = NULL;
	
	for(int i=0;i<TRANS_TABLE_SIZE;i++) {
		if(arp_tables[i].ip == 0) {
			table = &arp_tables[i];
			break;
		}
		if(arp_tables[i].age >= age) {
			age = arp_tables[i].age;
			table = &arp_tables[i];
		}
	}

	if(table) {
		table->ip = ip;
		memcpy(&table->mac[0], mac, 6);
		table->protocol = protocol;
		table->age = 0;
	}
}

//TODO need to call aging every 10 sec
void trans_table_aging(void)
{
	for(int i=0;i<TRANS_TABLE_SIZE;i++) {
		if(arp_tables[i].ip != 0)
			arp_tables[i].age++;
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
	struct arp_trans_table *table = trans_table_lookup(header->sender_ip);
	int merge_flag = 0;
	if(table && table->protocol == header->protocol_type) {
		memcpy(table->mac, header->sender_mac, 6);
		merge_flag = 1;
	}

	//check if arp packet is for us
	if(ipv4_info.ip == header->target_ip) {
		//update translate table
		if(!merge_flag)
			trans_table_insert(header->sender_ip,
					&header->sender_mac[0],
					header->protocol_type);

		switch(header->operation) {
			case ARP_REQUEST:
				header->hardware_type = htons(header->hardware_type);
				header->protocol_type = htons(header->protocol_type);
				header->operation = htons(ARP_REPLY);
				header->target_ip = ntohl(header->sender_ip);
				memcpy(header->target_mac, header->sender_mac, 6);
				header->sender_ip = ntohl(ipv4_info.ip);
				memcpy(header->sender_mac, mac_address, 6);

				netdev_write(buffer, size, ether->src_mac, ETH_TYPE_ARP);

				break;
			default:
				break;
		}
	}
	
	free(buffer);
	return 0;
}

void arp_request(uint32_t target_ip)
{
	struct ether_frame *ether = (struct ether_frame *)calloc(1, ETH_HEADER_SIZE + ARP_HEADER_SIZE);
	struct arp_header *header = (struct arp_header *)(&ether->data[0]);

	header->hardware_type = htons(ARP_ETHERNET);
	header->protocol_type = htons(ARP_IPV4);
	header->hardware_len = 6;
	header->protocol_len = 4;
	header->operation = htons(ARP_REQUEST);
	memcpy(header->sender_mac, mac_address, 6);
	header->sender_ip = htonl(ipv4_info.ip);
	memset(header->target_mac, 0xff, 6);
	header->target_ip = htonl(target_ip);

	netdev_write((uint8_t *)ether, ETH_HEADER_SIZE+ARP_HEADER_SIZE, BROADCAST_MAC, ETH_TYPE_ARP);

	free(ether);
}

int arp_init()
{
}

int arp_close()
{
}
