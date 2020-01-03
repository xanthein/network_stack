#ifndef ETH_H
#define ETH_H

#include <stdint.h>

struct ether_frame {
    uint8_t  dst_mac[6];
    uint8_t  src_mac[6];
    uint16_t type;
    uint8_t  data[];
} __attribute__((packed));

#define ETH_HEADER_SIZE sizeof(struct ether_frame)

#define ETH_TYPE_IP		0x0800
#define ETH_TYPE_ARP	0x0806

#endif
