#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "common.h"
int main(int c, char *z[])
{
	int	packet_socket;
	errn1(packet_socket = socket(AF_PACKET, SOCK_RAW, rev2(0x0806)), "open socket error");
	struct sockaddr_ll	sa_ll_b, sa_ll_s;
	sa_ll_b.sll_family = sa_ll_s.sll_family = AF_PACKET;
	sa_ll_b.sll_protocol = rev2(0x0806), sa_ll_b.sll_ifindex = get_ifindex(packet_socket, "eth0");
	sa_ll_s.sll_halen = 0x06, sa_ll_s.sll_ifindex = get_ifindex(packet_socket, "eth0"), strcpy(sa_ll_s.sll_addr, "\xff\xff\xff\xff\xff\xff");
	ok0(bind(packet_socket, (struct sockaddr*)&sa_ll_b, sizeof(struct sockaddr_ll)), "bind error");
	char	msg[1500+14];
	int	len = mk_arp_request_packet(msg, "\xcc\xcc\xcc\xcc\xcc\xcc", conv_ip("1.1.1.1"), conv_ip(z[1]));
	errn1(sendto(packet_socket, msg, len, 0, (struct sockaddr*)&sa_ll_s, sizeof(struct sockaddr_ll)), "sendto error");
	while (1) {
		errn1(recvfrom(packet_socket, msg, 1514, 0, NULL, NULL), "recvfrom error");
		if (*((int*)(msg+28)) == conv_ip(z[1])) {
			int	i;
			for (i=0;i<6;i++)
				printf("%.2x%c", msg[22+i]&0xff, (i+1==6)?'\n':':');
			return 0;
		}
	}
}
