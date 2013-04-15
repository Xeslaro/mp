#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "common.h"
int main(void)
{
	int	packet_socket;
	errn1(packet_socket = socket(AF_PACKET, SOCK_RAW, rev2(ETH_P_ALL)), "open socket error");
	struct sockaddr_ll	sa_ll;
	sa_ll.sll_family = AF_PACKET, sa_ll.sll_protocol = rev2(ETH_P_ALL), sa_ll.sll_ifindex = get_ifindex(packet_socket, "eth0");
	ok0(bind(packet_socket, (struct sockaddr*)&sa_ll, sizeof(struct sockaddr_ll)), "bind error");
	set_promiscuous(packet_socket, "eth0");
	int	len;
	char	msg[1514];
	while (1) {
		errn1(len = recvfrom(packet_socket, msg, 1514, 0, NULL, NULL), "recvfrom error");
		puts("msg received:");
		int	i;
		for (i=0;i<len;i++)
			printf("%.2x%c", msg[i]&0xff, (i==len-1)?'\n':' ');
		putchar('\n');
	}
	return 0;
}
