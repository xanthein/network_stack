#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>

struct ipv4_info_t {
	uint32_t ip;
};

struct ipv4_info_t ipv4_info;

void ipv4_init();

#endif
