#include <sys/socket.h>
#include <sys/types.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#define ok0(r, m) if (r) {\
	perror(m);\
	exit(-1);\
}
#define errn1(r, m) if (r==-1) {\
	perror(m);\
	exit(-1);\
}
int main(void)
{
	int		packet_socket = socket(AF_PACKET, SOCK_RAW, 0x0806);
	struct ifreq	eth_if;
	strcpy(eth_if.ifr_name, "eth0");
	ioctl(packet_socket, SIOCGIFINDEX, &eth_if);
	struct sockaddr_ll	sa_ll;
	memset(&sa_ll, 0, sizeof(struct sockaddr_ll));
	sa_ll.sll_protocol = 0x0806, sa_ll.sll_ifindex = eth_if.ifr_ifindex, sa_ll.sll_halen = 6;
	strcpy(sa_ll.sll_addr, "\xff\xff\xff\xff\xff\xff");
	char	msg[1526];
	*((int*)msg) = 0x55555555;
	*((int*)(msg+4)) = 0xd5555555;
	*((int*)(msg+8)) = 0xffffffff; *(msg+12) = 0xff; *(msg+13) = 0xff;
	*((int*)(msg+14)) = 0xcc88cc88; *(msg+18) = 0x88; *(msg+19) = 0xcc;
	*(msg+20) = 0x08; *(msg+21) = 0x06;
	*(msg+22) = 0x00, *(msg+23) = 0x01;
	*(msg+24) = 0x08, *(msg+25) = 0x00;
	*(msg+26) = 0x06, *(msg+27) = 0x04;
	*(msg+28) = 0x00, *(msg+29) = 0x02;
	*((int*)(msg+30)) = 0xcc88cc88, *(msg+34) = 0x88, *(msg+35) = 0xcc;
	*((int*)(msg+36)) = 0x196b930a;
	*((int*)(msg+40)) = 0x6afce000, *(msg+44) = 0xa6, *(msg+45) = 0xd0;
	*((int*)(msg+46)) = 0x016b930a;
	*((int*)(msg+50)) = 0x645aebcf;
	while (1) {
		errn1(sendto(packet_socket, msg+8, 50-8, 0, (struct sockaddr*)&sa_ll, sizeof(struct sockaddr_ll)), "sendto error");
		sleep(10);
	}
}
