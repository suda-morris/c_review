/*
 * 	Net_Multicast.c
 *
 *  Created on: 2016年11月19日
 *      Author: morris
 *  要求：
 *  	网络编程之---组播通信
 *  	实现组播的发送与接收
 *	********************************************************************
 *	1. 广播会占用网络带宽，造成广播风暴，影响正常通信
 *	2. 组播是一种折中的方式，只有加入某个组播组的主机才能收到数据
 *	3. 组播方式既可以发给多个主机，又能避免像广播那样带来过多的负载（每台主机要到传输层
 *	才能判断广播包是否要处理）
 *	4. 广播只能面向同网段下的所有主机，同网段外的其他主机想要接收则做不到。组播可以跨网段
 *	5.组播源不一定属于组播组，它向组播源发送数据，自己不一定是接受者，多个组播源可以向同
 *	一个组播组发送数据
 *	6. 组播组需要一个特殊的IP地址，D类IP地址用作组播地址，范围是：
 *	224.0.0.1~239.255.255.254（中间除掉广播地址）。其中
 *	239.0.0.1~239.255.255.254为私有组播地址，用作局域网通信。
 *	7. D类IP地址不用来标志任何主机而用来标志组播组。接收方必须提前加入某个组播组才能接收
 *	8. 组播的发送：
 *		a. 创建用户数据报套接字
 *		b. 接收方地址指定为组播地址
 *		c. 指定端口信息
 *		d. 发送数据包
 *	9. 组播的接收：
 *		a. 创建用户数据报套接字
 *		b. 加入组播组
 *		c. 绑定本机IP地址和端口号
 *		d. 等待接收数据
 *	********************************************************************
 *	加入组播组时需要使用一个struct mreqn结构体作为操作数传递给setsockopt函数
 *	struct ip_mreq
 *	{
 *		struct in_addr imr_multiaddr;//需要加入的组播组地址
 *		struct in_addr imr_interface;//本地IP地址
 *	}
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MCAST_PORT			5006
#define MCAST_ADDR			"239.1.2.3"
#define BUFFER_SIZE			1024

/* 程序使用说明 */
void Usage(char* arg) {
	printf("Sender Usage:%s s/S\r\n", arg);
	printf("Receive Usage:%s r/R\r\n", arg);
}
int main(int argc, char **argv) {
	int sender_fd, receiver_fd;
	struct sockaddr_in sender_addr, receiver_addr, mcast_addr;
	socklen_t addrlen;
	struct ip_mreq mreq;
	char buf[BUFFER_SIZE];
	int real_read, real_write;
	int ret;
	/*参数检查*/
	if (argc <= 1) {
		Usage(argv[0]);
		exit(1);
	}
	/* 发送端 */
	if (strncasecmp(argv[1], "s", 1) == 0) {
		/* 创建数据报套接字 */
		sender_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sender_fd < 0) {
			perror("socket");
			exit(1);
		}
		/* 填充目标组播地址结构 */
		memset(&mcast_addr, 0, sizeof(mcast_addr));
		mcast_addr.sin_family = AF_INET;
		mcast_addr.sin_port = htons(MCAST_PORT);
		mcast_addr.sin_addr.s_addr = inet_addr(MCAST_ADDR);
		/* 循环发送组播数据 */
		while (fgets(buf, BUFFER_SIZE, stdin)) {
			real_write = sendto(sender_fd, buf, strlen(buf), 0,
					(struct sockaddr*) &mcast_addr, sizeof(mcast_addr));
			if (real_write < 0) {
				perror("sendto");
				exit(1);
			}
			if (strncmp(buf, "quit", 4) == 0) {
				break;
			}
		}
		close(sender_fd);
	}
	/* 接收方 */
	else if (strncasecmp(argv[1], "r", 1) == 0) {
		/* 创建数据报套接字 */
		receiver_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (receiver_fd < 0) {
			perror("socket");
			exit(1);
		}
		/* 加入组播组 */
		mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		ret = setsockopt(receiver_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
				sizeof(mreq));
		if (ret < 0) {
			perror("setsockopt");
			exit(1);
		}
		/* 接收方绑定本地IP地址和端口号 */
		memset(&receiver_addr, 0, sizeof(receiver_addr));
		receiver_addr.sin_family = AF_INET;
		receiver_addr.sin_port = htons(MCAST_PORT);
		receiver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		ret = bind(receiver_fd, (struct sockaddr*) &receiver_addr,
				sizeof(receiver_addr));
		if (ret < 0) {
			perror("bind");
			exit(1);
		}
		/* 循环接收组播源发送的消息 */
		while (1) {
			memset(buf, 0, BUFFER_SIZE);
			real_read = recvfrom(receiver_fd, buf, BUFFER_SIZE, 0,
					(struct sockaddr*) &sender_addr, &addrlen);
			if (real_read < 0) {
				perror("recvfrom");
				exit(1);
			}
			printf("Receive message from %s:%s\r\n",
					inet_ntoa(sender_addr.sin_addr), buf);
			if (strncmp(buf, "quit", 4) == 0) {
				break;
			}
		}
		close(receiver_fd);
	} else {
		Usage(argv[0]);
		exit(1);
	}
	return 0;
}

