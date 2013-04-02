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
	sa_ll.sll_protocol = 0x6488, sa_ll.sll_ifindex = eth_if.ifr_ifindex, sa_ll.sll_halen = 6;
	sa_ll.sll_addr[0] = 0x00;
	strcpy(sa_ll.sll_addr+1, "\xe0\xfc\x6a\xa6\xd0");
	char	msg[1526];
	*((int*)msg) = 0x55555555;
	*((int*)(msg+4)) = 0xd5555555;
	*((int*)(msg+8)) = 0x6afce000; *(msg+12) = 0xa6; *(msg+13) = 0xd0;
	*((int*)(msg+14)) = 0x92c8bcc8; *(msg+18) = 0xf2; *(msg+19) = 0x61;
	*(msg+20) = 0x88; *(msg+21) = 0x64;
	int	sid;
	scanf("%x", &sid);
	*(msg+22) = 0x11, *(msg+23) = 0xa7, *(msg+24) = sid>>8, *(msg+25) = sid;
	*(msg+26) = 0x00, *(msg+27) = 0x00;
	errn1(sendto(packet_socket, msg+8, 28-8, 0, (struct sockaddr*)&sa_ll, sizeof(struct sockaddr_ll)), "sendto error");
	return 0;
}
