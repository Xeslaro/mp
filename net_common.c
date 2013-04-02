#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
void set_promiscuous(int socket,  char *if_name)
{
	struct ifreq	eth_if;
	strcpy(eth_if.ifr_name, if_name);
	ioctl(socket, SIOCGIFFLAGS, &eth_if);
	if (!(eth_if.ifr_flags & IFF_PROMISC)) {
		eth_if.ifr_flags = IFF_PROMISC;
		ioctl(socket, SIOCSIFFLAGS, &eth_if);
	}
}
int get_ifindex(int socket, char *if_name)
{
	struct ifreq	eth_if;
	strcpy(eth_if.ifr_name, if_name);
	ioctl(socket, SIOCGIFINDEX, &eth_if);
	return eth_if.ifr_ifindex;
}
int conv_ip(char *s)
{
	int	a, b, c, d;
	sscanf(s, "%d.%d.%d.%d", &a, &b, &c, &d);
	return (d<<24) + (c<<16) + (b<<8) + a;
}
void conv_mac(char *s, char *z)
{
	int	i;
	for (i=0;i<6;i++) {
		int	k;
		sscanf(s+i*3, "%x", &k);
		z[i] = k;
	}
}
/* ip is already converted to network byte order */
int mk_arp_reply_packet(char *msg, char *mac_dest, char *mac_src, int ip_dest, int ip_src, int greeting)
{
	int	i;
	for (i=0;i<6;i++)
		msg[i]=mac_dest[i], msg[i+6]=mac_src[i];
	*((short*)(msg+12)) = 0x0608;
	*((short*)(msg+14)) = 0x0100;
	*((short*)(msg+16)) = 0x0008;
	msg[18] = 0x06, msg[19] = 0x04;
	*((short*)(msg+20)) = 0x0200;
	for (i=0;i<6;i++) {
		msg[22+i]=mac_src[i];
		msg[32+i]=greeting?mac_src[i]:mac_dest[i];
	}
	*((int*)(msg+28)) = ip_src;
	*((int*)(msg+38)) = greeting?ip_src:ip_dest;
	return 42;
}
int mk_padt_packet(char *msg, char *mac_dest, char *mac_src, int sid)
{
	int	i;
	for (i=0;i<6;i++)
		msg[i]=mac_dest[i], msg[i+6]=mac_src[i];
	*((short*)(msg+12)) = 0x6488;
	msg[14]=0x11, msg[15]=0xa7;
	msg[16]=sid>>8, msg[17]=sid;
	*((short*)(msg+18)) = 0x0000;
	return 20;
}
