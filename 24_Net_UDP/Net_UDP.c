/*
 * Net_UDP.c
 *
 *  Created on: 2016年11月17日
 *      Author: morris
 *  要求：
 *  	UDP编程---服务器+客户端
 *  	建立基于UDP协议的服务器与客户端的通讯，客户端向服务器发送字符串，服务器将收到
 *  	的字符串打印出来
 *	********************************************************************
 *	1. 在服务端程序中，要进行UDP通讯需要经过如下几个步骤
 *		a. 通过socket创建一个数据报套接字
 *		b. 通过bind将套接字与服务器地址绑定
 *		c. 使用recvfrom/sendto等函数，直接接收消息或向某个地址发送消息
 *		d. 使用close关闭套接字
 *	2. 在客户端程序中，要与服务器进程UDP通讯
 *		a. 通过socket创建一个数据报套接字
 *		b. 使用sendto等函数，直接向某个地址发送消息。如果要通过recvfrom接收消息，也
 *		需要将套接字绑定本地主机的网络地址上
 *		c. 使用close关闭套接字
 *	********************************************************************
 * 	int sendto(int sockfd,const void* msg,int len,unsigned int flags,
 const struct sockaddr* to,int tolen);发送通过远端主机指定的套接字数据
 *		sockfd:连接至远端主机的套接字描述符
 *		msg:指向要发送数据的指针
 *		len:数据长度
 *		flags:操作标志，一般为0
 *			MSG_DONTWAIT:非阻塞发送
 *			MSG_NOSIGNAL:不会被SIGPIPE信号中断
 *		to:目的地址的IP地址和端口号信息
 *		tolen:接收端的地址结构长度
 *		返回：
 *			成功：返回实际发送的字节数
 *			出错：返回-1,并设置errno
 *	int recvfrom(int sockfd,void* buf,int len,unsigned int flags,
 struct sockaddr* from,int* fromlen);经socket接收数据
 *		sockfd:接收端套接字描述符
 *		buf:存放接收数据的缓冲区
 *		len:接收缓冲区长度
 *		flags:操作标志，一般为0
 *			MSG_PEEK:返回的数据不会在系统内删除
 *			MSG_WAITALL:强迫接收到len大小的数据后才返回
 *			MSG_NOSIGNAL:不会被SIGPIPE信号中断
 *		from:记录源主机的IP地址和端口号信息
 *		fromlen:发送端地址结构的长度
 *		返回：
 *			成功：返回实际接收到的字节数
 *			出错：返回-1，并设置errno
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT				4321
#define BUFFER_SIZE			1024

/* 程序使用说明 */
void Usage(char* arg) {
	printf("Usage:%s s/S\r\nUsage:%s c/C ipaddr\r\n", arg, arg);
}

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in server_addr, client_addr;
	int addrlen;
	char buf[BUFFER_SIZE];
	int real_read;
	int ret;

	/* 参数检查 */
	if (argc <= 1) {
		Usage(argv[0]);
		exit(1);
	}
	/* 建立数据报套接字 */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		perror("socket");
		exit(1);
	}
	/* 服务端 */
	if (strncasecmp(argv[1], "s", 1) == 0) {
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(PORT);
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		/* 绑定套接字与服务器地址、端口号 */
		ret = bind(sockfd, (struct sockaddr*) &server_addr,
				sizeof(server_addr));
		if (ret < 0) {
			perror("bind");
			exit(1);
		}
		/* 循环读取客户端发送的数据 */
		while (1) {
			memset(buf, 0, BUFFER_SIZE);
			addrlen = sizeof(client_addr);
			real_read = recvfrom(sockfd, buf, BUFFER_SIZE, 0,
					(struct sockaddr*) &client_addr, (socklen_t*) &addrlen);
			if (real_read == -1) {
				perror("recvfrom");
				exit(1);
			}
			buf[real_read] = '\0';
			printf("Receive message from %s:%s\r\n",
					inet_ntoa(client_addr.sin_addr), buf);
			if (strncmp(buf, "quit", 4) == 0) {
				break;
			}
		}
	}
	/* 客户端 */
	else if (strncasecmp(argv[1], "c", 1) == 0) {
		if (argc <= 2) {
			Usage(argv[0]);
			exit(1);
		}
		/* 构造服务器资料 */
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(PORT);
		inet_aton(argv[2], &server_addr.sin_addr);
		/* 循环发送数据给服务器 */
		while (1) {
			memset(buf, 0, BUFFER_SIZE);
			printf("Input:\r\n");
			fgets(buf, BUFFER_SIZE, stdin);
			sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr*) &server_addr,
					sizeof(server_addr));
			if (strncmp(buf, "quit", 4) == 0) {
				break;
			}
		}
	} else {
		Usage(argv[0]);
		exit(1);
	}
	/* 关闭套接字 */
	close(sockfd);
	return 0;
}

