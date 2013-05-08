#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include "common.h"
#define net_if "eth0"
typedef struct send_eth_msg_info {
	int	socket, len;
	char	*msg;
	struct sockaddr_ll	*sa_ll;
}semi;
sem_t	sem_info;
int	f, packet_socket;
void alarm_handler(int signum)
{
	if (f)
		leave_promiscuous(packet_socket, net_if);
	exit(0);
}
void* send_eth_packet(void *a)
{
	semi	*p = (semi*)a;
	ok0(sem_wait(&sem_info), "sem_wait error");
	errn1(sendto(p->socket, p->msg, p->len, 0, (struct sockaddr*)p->sa_ll, sizeof(struct sockaddr_ll)), "sendto error");
}
int main(int c, char *z[])
{
	errn1(packet_socket = socket(AF_PACKET, SOCK_RAW, rev2(ETH_P_ALL)), "open socket error");
	struct sockaddr_ll	sa_ll_b, sa_ll_s;
	sa_ll_b.sll_family = sa_ll_s.sll_family = AF_PACKET;
	sa_ll_b.sll_protocol = rev2(ETH_P_ALL), sa_ll_b.sll_ifindex = get_ifindex(packet_socket, net_if);
	sa_ll_s.sll_halen = 0x06, sa_ll_s.sll_ifindex = get_ifindex(packet_socket, net_if), strcpy(sa_ll_s.sll_addr, "\xff\xff\xff\xff\xff\xff");
	ok0(bind(packet_socket, (struct sockaddr*)&sa_ll_b, sizeof(struct sockaddr_ll)), "bind error");
	char	msg[1500+14];
	int	len = mk_arp_request_packet(msg, "\xcc\xcc\xcc\xcc\xcc\xcc", conv_ip("1.1.1.1"), conv_ip(z[1]));
	semi	a;
	a.socket = packet_socket, a.len = len, a.msg = msg, a.sa_ll = &sa_ll_s;
	pthread_t	thread_info;
	ok0(sem_init(&sem_info, 0, 0), "sem_init error");
	ok0(pthread_create(&thread_info, NULL, send_eth_packet, (void*)&a), "pthread_create error");
	f = set_promiscuous(packet_socket, net_if);
	signal(SIGALRM, alarm_handler);
	alarm(2);
	while (1) {
		ok0(sem_post(&sem_info), "sem_post error");
		errn1(recvfrom(packet_socket, msg, 1514, 0, NULL, NULL), "recvfrom error");
		if (*((short*)(msg+12)) == rev2(0x0806) && *((int*)(msg+28)) == conv_ip(z[1])) {
			int	i;
			for (i=0;i<6;i++)
				printf("%.2x%c", msg[22+i]&0xff, (i+1==6)?'\n':':');
			if (f)
				leave_promiscuous(packet_socket, net_if);
			return 0;
		}
	}
}
