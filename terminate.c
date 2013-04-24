#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#define net_if "qemu_eth0"

int main(int c, char *z[])
{
	int	packet_socket;
	errn1(packet_socket = socket(AF_PACKET, SOCK_RAW, rev2(0x8864)), "socket error");
	char	pkt[14+1500+1500], *p = pkt+1500, *h = pkt+1500;
	p[0] = 0x05, p[1] = 0xcc, *((short*)(p+2)) = rev2(0x0004); p+=4;
	h -= encap_ppp(rev2(0xc021), h);
	short	session_id;
	sscanf(z[1], "%hx", &session_id);
	h -= encap_pppoe(0x00, rev2(session_id), rev2(p-h), h);
	char	mac_src[6], mac_dst[6];
	conv_mac(z[2], mac_src);
	conv_mac(z[3], mac_dst);
	h -= encap_ether(mac_dst, mac_src, rev2(0x8864), h);
	struct sockaddr_ll	sa_ll_s;
	sa_ll_s.sll_family = AF_PACKET, sa_ll_s.sll_halen = 6, sa_ll_s.sll_ifindex = get_ifindex(packet_socket, net_if);
	memcpy(mac_dst, sa_ll_s.sll_addr, 6);
	errn1(sendto(packet_socket, h, p-h, 0, (struct sockaddr*)&sa_ll_s, sizeof(struct sockaddr_ll)), "sendto error");
	return 0;
}
