#include <stdint.h>
#include "utils.h"

uint16_t calculate_checksum(uint8_t *buffer, uint32_t size)
{
	uint32_t sum = 0;
	uint16_t *buffer16 = (uint16_t *)buffer;


	for(int i=0;i<size/2;i++)
		sum += buffer16[i];

	return ~((sum>>16) + (sum&0xffff));
}
