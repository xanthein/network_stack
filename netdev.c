#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include "tap_if.h"
#include "ethernet.h"
#include "netdev.h"
#include "arp.h"
#include "ipv4.h"

void netdev_read(void)
{
	int size = 0;

	uint8_t *buffer	= calloc(1, NETDEV_BUFFER_LEN);
	
	size = tap_read(buffer, NETDEV_BUFFER_LEN);
	struct ether_frame *frame = (struct ether_frame *)buffer;

	switch(ntohs(frame->type)) {
		case ETH_TYPE_IP:
			printf("receive IP packet\n");
			break;
		case ETH_TYPE_ARP:
			printf("receive ARP packet\n");
			arp_read(buffer, size);
			break;
		default:
			printf("ethernet type 0x%x not support\n", frame->type);
			break;
	}
}

void netdev_write(uint8_t *buffer, size_t size)
{
	tap_write(buffer, size);
}

int netdev_open(void)
{
	tap_init();
	ipv4_init();
	arp_init();
}

int netdev_close(void)
{
	tap_close();
}
