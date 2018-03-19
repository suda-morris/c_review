/*
 * 	NTP_Client.c
 *
 *  Created on: 2016年11月20日
 *      Author: morris
 *  要求：
 *  	网络编程之---NTP客户端程序
 *  	本程序需要使用超级用户权限运行，测试方法：
 *  	a. 为本地设定一个错误的时间：sudo date -s "2001-01-01 1:00:00"
 *  	b. 运行本程序
 *	********************************************************************s
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define NTP_PORT_STR			"123"//NTP协议专用端口号
#define NTP_SERVER_ADDR_STR		"ntp.suda.edu.cn"//NTP服务器地址
#define TIME_OUT				(10)//一次NTP时间同步延迟最长时间10s
#define NTP_PCK_LEN				(48)//NTP协议数据报长度
#define LI						(0)//警告在当月的最后一天的最终时刻插入的迫近闰秒
#define VN						(3)//版本号
#define MODE					(3)//工作模式
#define STRATUM					(0)//对本地时钟的整体识别
#define POLL					(4)//连续信息间的最大间隔
#define	PREC					(-6)//本地时钟精确度
#define JAN_1970				(0x83aa7e80)//从1900年到1970年的时间秒数
#define NTPFRAC(x)				(4294*(x)-759*((((x)>>10)+32768)>>16))
#define USEC(x)					(((x)>>12)-759*((((x)>>10)+32768)>>16))

typedef struct _ntp_time {
	unsigned int coarse;
	unsigned int fine;
} ntp_time;

struct ntp_packet {
	unsigned char leap_ver_mode;
	unsigned char startum;
	char poll;
	char precision;
	int root_delay;
	int root_dispersion;
	int reference_identifier;
	ntp_time reference_timestamp;
	ntp_time originage_timestamp;
	ntp_time receive_timestamp;
	ntp_time transmit_timestamp;
};

/* 构造NTP协议包 */
int construct_packet(char* packet) {
	long tmp_wrd;
	time_t timer;
	memset(packet, 0, NTP_PCK_LEN);
	/* 设置16字节的包头 */
	tmp_wrd = htonl(
			(LI << 30) | (VN << 27) | (MODE << 24) | (STRATUM << 16)
					| (POLL << 8) | (PREC & 0xff));
	memcpy(packet, &tmp_wrd, sizeof(tmp_wrd));
	/* 设置Root Delay,Root Dispersion和Reference Indentifier */
	tmp_wrd = htonl(1 << 16);
	memcpy(&packet[4], &tmp_wrd, sizeof(tmp_wrd));
	memcpy(&packet[8], &tmp_wrd, sizeof(tmp_wrd));
	/* 设置Timestamp部分 */
	time(&timer);
	/* 设置Transmit Timestamp coarse */
	tmp_wrd = htonl(JAN_1970 + (long) timer);
	memcpy(&packet[40], &tmp_wrd, sizeof(tmp_wrd));
	/* 设置Transmit Timestamp fine */
	tmp_wrd = htonl((long) NTPFRAC(timer));
	memcpy(&packet[44], &tmp_wrd, sizeof(tmp_wrd));
	return NTP_PCK_LEN;
}

/* 通过网络从NTP服务器获取NTP时间 */
int get_ntp_time(int socket_fd, struct addrinfo* addr,
		struct ntp_packet *ret_time) {
	fd_set inset;
	struct timeval tv;
	char buf[NTP_PCK_LEN * 2];
	int addr_len = addr->ai_addrlen;
	int packet_len;
	int real_send, real_read;
	int ret;
	/* 构造NTP协议包 */
	packet_len = construct_packet(buf);
	if (packet_len == 0) {
		printf("construct_packet error\r\n");
		return -1;
	}
	/* 将NTP协议数据包发送给NTP服务器 */
	real_send = sendto(socket_fd, buf, packet_len, 0, addr->ai_addr, addr_len);
	if (real_send < 0) {
		perror("sendto");
		return -1;
	}
	/* 使用select函数监听,设定超时时间 */
	FD_ZERO(&inset);
	FD_SET(socket_fd, &inset);
	tv.tv_sec = TIME_OUT;
	tv.tv_usec = 0;
	ret = select(socket_fd + 1, &inset, NULL, NULL, &tv);
	if (ret < 0) {
		perror("select");
		return -1;
	} else if (ret == 0) {
		printf("Time-out\r\n");
		return -1;
	}
	/* 接收服务器的响应数据 */
	real_read = recvfrom(socket_fd, buf, sizeof(buf), 0, addr->ai_addr,
			(socklen_t*) &addr_len);
	if (real_read < 0) {
		printf("recvfrom error\r\n");
		return -1;
	}
	if (real_read < NTP_PCK_LEN) {
		printf("Receive packet wrong\r\n");
		return -1;
	}
	/* 设置接收NTP协议数据包的数据结构 */
	ret_time->leap_ver_mode = ntohl(buf[0]);
	ret_time->startum = ntohl(buf[1]);
	ret_time->poll = ntohl(buf[2]);
	ret_time->precision = ntohl(buf[3]);
	ret_time->root_delay = ntohl(*(int*) &(buf[4]));
	ret_time->root_dispersion = ntohl(*(int*) &(buf[8]));
	ret_time->reference_identifier = ntohl(*(int*) &(buf[12]));
	ret_time->reference_timestamp.coarse = ntohl(*(int*) &(buf[16]));
	ret_time->reference_timestamp.fine = ntohl(*(int*) &(buf[20]));
	ret_time->originage_timestamp.coarse = ntohl(*(int*) &(buf[24]));
	ret_time->originage_timestamp.fine = ntohl(*(int*) &(buf[28]));
	ret_time->receive_timestamp.coarse = ntohl(*(int*) &(buf[32]));
	ret_time->receive_timestamp.fine = ntohl(*(int*) &(buf[36]));
	ret_time->transmit_timestamp.coarse = ntohl(*(int*) &(buf[40]));
	ret_time->transmit_timestamp.fine = ntohl(*(int*) &(buf[44]));
	return 0;
}

/* 根据NTP数据包信息更新本地的当前时间 */
int set_local_time(struct ntp_packet* pnew_time_packet) {
	struct timeval tv;
	tv.tv_sec = pnew_time_packet->transmit_timestamp.coarse - JAN_1970;
	tv.tv_usec = USEC(pnew_time_packet->transmit_timestamp.fine);
	return settimeofday(&tv, NULL);
}

int main(int argc, char **argv) {
	int sockfd;
	struct addrinfo hints, *res = NULL;
	struct ntp_packet new_time_packet;
	int ret;
	/* 获取NTP服务器地址信息 */
	hints.ai_family = AF_UNSPEC;	//IPV4,IPv6都行
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	ret = getaddrinfo(NTP_SERVER_ADDR_STR, NTP_PORT_STR, &hints, &res);
	if (ret != 0) {
		perror("getaddrinfo");
		exit(1);
	}
	/* 打印NTP服务器的IP地址信息 */
	printf("NTP server address:%s\r\n",
			inet_ntoa(((struct sockaddr_in*) res->ai_addr)->sin_addr));
	/* 创建套接字 */
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(1);
	}
	/* 从NTP服务器获取时间 */
	ret = get_ntp_time(sockfd, res, &new_time_packet);
	if (ret < 0) {
		printf("get_ntp_time error\r\n");
		exit(1);
	}
	/* 调整本地时间 */
	ret = set_local_time(&new_time_packet);
	if (ret == 0) {
		printf("NTP client success\r\n");
	} else {
		printf("set_local_time error,try sudo...\r\n");
		exit(1);
	}
	close(sockfd);
	return 0;
}

