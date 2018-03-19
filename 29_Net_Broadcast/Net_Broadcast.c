/*
 * 	Net_Broadcast.c
 *
 *  Created on: 2016年11月19日
 *      Author: morris
 *  要求：
 *  	网络编程之---广播通信
 *  	实现广播的发送与接收
 *	********************************************************************
 *	1. 广播时面向整个网段的，某太主机发送广播后在同一网段的所有主机都能收到广播，但是
 *	其他网段的主机则无法接收。
 *	2. 广播是一对多通信，因此面向连接的流式套接字无法发送广播，只有用户数据报套接字可以
 *	3. Linux系统默认是不允许用户发送广播的，可以通过setsockopt函数来设定套接字的属性
 *	开启广播
 *	4. 255.255.255.255在所有网段中都代表广播地址,大部分的路由器会禁止这个地址
 *	5. 广播的发送：
 *		a. 创建用户数据报套接字
 *		b. 缺省创建的套接字不允许广播数据报，需要设置套接字属性
 *		c. 接收方地址指定为广播地址
 *		d. 指定端口信息
 *		e. 发送数据包
 *	6. 广播的接收：
 *		a. 创建用于数据报套接字
 *		b. 绑定本地IP地址和端口号：绑定的端口必须和发送方指定的端口相同
 *		c. 等待接收数据
 *	********************************************************************
 *	int getsockopt(int sockfd, int level, int optname,
 void *optval, socklen_t *optlen);获取套接字属性
 *	int setsockopt(int sockfd, int level, int optname,
 const void *optval, socklen_t optlen);设置套接字属性
 *	 	sockfd:套接字描述符
 *		level:指定控制套接字的层次
 *			SOL_SOCKET:通用套接字选项
 *			IPPROTO_IP:IP选项
 *			IPPROTO_TCP:TFP选项
 *			IPPROTO_UDP:UDP选项
 *		optname:操作名称,每层协议的属性各不相同
 *			对于SOL_SOCKET
 *				SO_BROADCAST，int类型，允许发送广播数据
 *				SO_DEBUG，int类型，允许调试
 *				SO_DONTROUTE，int类型，不差找路由
 *				SO_ERROR，int类型，获得套接字错误
 *				SO_KEEPALIVE，int类型，保持连接
 *				SO_LINGER，struct linger类型，延迟关闭连接
 *				SO_OOBINLINE，int类型，待外数据放入正常数据流
 *				SO_RCVBUF，int类型，接收缓冲区大小
 *				SO_SNDBUF，int类型，发送缓冲区大小
 *				SO_RCVLOWAT，int类型，接收缓冲区下限
 *				SO_SNDLOWAT，int类型，发送缓冲区下限
 *				SO_RCVTIMEO，struct timeval类型，接收超时
 *				SO_SNDTIMEO，struct timeval类型，发送超时
 *				SO_REUSERADDR，int类型，允许重用本地地址和端口
 *				SO_TYPE，int类型，获得套接字类型
 *				SO_BSDCOMPAT，int类型，与BSD系统兼容
 *			对于IPPROTO_IP
 *				IP_HDRINCL，int类型，在数据包中包含IP首部
 *				IP_OPTINOS，int类型，IP首部选项
 *				IP_TOS，入伍类型
 *				IP_TTL，int类型，生存时间
 *			对于IPPROTO_TCP
 *				TCP_MAXSEG，int类型，TCP最大数据段大小
 *				TCP_NODELAY，int类型，不使用Nagle算法
 *		optval:操作值
 *		optlen:optval的长度
 *		返回：
 *			成功：返回0
 *			出错：返回-1，并设置errno
 */

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BCAST_PORT			5005
#define BCAST_ADDR			"10.0.2.255"
#define BUFFER_SiZE			1024

/* 程序使用说明 */
void Usage(char* arg) {
	printf("Sender Usage:%s s/S\r\n", arg);
	printf("Receive Usage:%s r/R\r\n", arg);
}

int main(int argc, char **argv) {
	int sender_fd, receiver_fd;
	struct sockaddr_in sender_addr, bcast_addr;
	socklen_t addr_len;
	char buf[BUFFER_SiZE];
	int b_broadcast;
	int real_write, real_read;
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
		/* 开启广播属性 */
		b_broadcast = 1;
		ret = setsockopt(sender_fd, SOL_SOCKET, SO_BROADCAST, &b_broadcast,
				sizeof(b_broadcast));
		if (ret < 0) {
			perror("setsockopt");
			exit(1);
		}
		/* 接收方地址设定为广播地址 */
		memset(&bcast_addr, 0, sizeof(bcast_addr));
		bcast_addr.sin_family = AF_INET;
		bcast_addr.sin_port = htons(BCAST_PORT);
		ret = inet_aton(BCAST_ADDR, &bcast_addr.sin_addr);
		if (ret == 0) {
			perror("inet_aton");
			exit(1);
		}
		/* 循环发送广播 */
		while (fgets(buf, BUFFER_SiZE, stdin)) {
			real_write = sendto(sender_fd, buf, strlen(buf), 0,
					(struct sockaddr*) &bcast_addr, sizeof(bcast_addr));
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
		/* 接收方绑定广播地址 */
		memset(&bcast_addr, 0, sizeof(bcast_addr));
		bcast_addr.sin_family = AF_INET;
		bcast_addr.sin_port = htons(BCAST_PORT);
#if 1
		bcast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
#else
		ret = inet_aton(BCAST_ADDR, &bcast_addr.sin_addr);
		if (ret == 0) {
			perror("inet_aton");
			exit(1);
		}
#endif
		ret = bind(receiver_fd, (struct sockaddr*) &bcast_addr,
				sizeof(bcast_addr));
		if (ret < 0) {
			perror("bind");
			exit(1);
		}
		/* 循环接收广播发送方传送的消息 */
		while (1) {
			memset(buf, 0, BUFFER_SiZE);
			real_read = recvfrom(receiver_fd, buf, BUFFER_SiZE, 0,
					(struct sockaddr*) &sender_addr, &addr_len);
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

