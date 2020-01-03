#include <stdio.h>
#include "netdev.h"

int main()
{
	netdev_open();
	for(int i=0;i<100;i++) {
		netdev_read();
	}
	netdev_close();
}
