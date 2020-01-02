#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#define TAP_IP_ADDR "10.0.0.5"
#define TAP_IP_MASK "255.255.255.255"
#define TAP_ROUTE_ADDR "10.0.0.0"
#define TAP_ROUTE_MASK "255.255.255.0"

static int tap_fd;
static char* dev;

static int set_address(char *name, char *address, char *netmask)
{
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	struct ifreq ifr;
	strncpy(ifr.ifr_name, name, IFNAMSIZ);

	ifr.ifr_addr.sa_family = AF_INET;
	struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
	inet_pton(AF_INET, address, &addr->sin_addr);
	if(ioctl(fd, SIOCSIFADDR, &ifr))
		perror("set address failed\n");
	
	inet_pton(AF_INET, netmask, &addr->sin_addr);
	if(ioctl(fd, SIOCSIFNETMASK, &ifr))
		perror("set netmask failed\n");

	ifr.ifr_flags |= IFF_UP;
	if(ioctl(fd, SIOCSIFFLAGS, &ifr))
		perror("enable device failed\n");

	close(fd);

	return 0;
}

static int set_route(char *name, char *address, char *mask)
{
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
 
	struct rtentry route;
	memset(&route, 0, sizeof(route));

	struct sockaddr_in *addr = (struct sockaddr_in *)&route.rt_gateway;
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = 0;

	addr = (struct sockaddr_in*) &route.rt_dst;
	addr->sin_family = AF_INET;
	inet_pton(AF_INET, address, &addr->sin_addr);

	addr = (struct sockaddr_in*) &route.rt_genmask;
	addr->sin_family = AF_INET;
	inet_pton(AF_INET, mask, &addr->sin_addr);
	
	route.rt_dev = name;
	route.rt_flags = RTF_UP;
	route.rt_metric = 0;

	if(ioctl(fd, SIOCADDRT, &route))
		perror("set route failed\n");

	close( fd );
	return 0; 
}

static int tap_alloc(char *dev)
{
    struct ifreq ifr;
    int fd, err;
    
	if((fd = open("/dev/net/tap", O_RDWR)) < 0 ) {
		perror("Open /dev/net/tap failed\n");
		exit(1);
	}
    
    memset(&ifr, 0, sizeof(struct ifreq));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
		perror("Can't run ioctl tap\n");
		close(fd);
		return err;
	}
    
	strcpy(dev, ifr.ifr_name);
    return fd;
}

int tap_init()
{
	dev = calloc(10, 1);
	tap_fd = tap_alloc(dev);

	set_address(dev, TAP_IP_ADDR, TAP_IP_MASK);
	set_route(dev, TAP_ROUTE_ADDR, TAP_ROUTE_MASK);
}

int tap_close()
{
	free(dev);
	close(tap_fd);
}

int tap_read(char *buf, int len)
{
    return read(tap_fd, buf, len);
}

int tap_write(char *buf, int len)
{
    return write(tap_fd, buf, len);
}
