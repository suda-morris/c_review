/*
 * 	Net_Nonblock.c
 *
 *  Created on: 2016年11月18日
 *      Author: morris
 *  要求：
 *  	网络编程之---阻塞式IO
 *	********************************************************************
 *	1. 缺省模式下，套接字建立后所处于的模式就是阻塞I/O模式
 *	2. 常见的会阻塞的函数有：
 *		a. 读操作：read,recv,recvfrom
 *		b. 写操作：write,send
 *		c. 其他操作：accept,connect
 *	3. UDP不用等待确认，没有实际的发送缓冲区，所以UDP协议中不存在发送缓冲区满的情况，在
 *	UDP套接字上执行的写操作永远都不会阻塞
 *	********************************************************************
 */

#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT				5001
#define MAX_QUE_CONN_NUM	5
#define BUFFER_SIZE			1024

void Usage(char* arg) {
	printf("Usage:%s s/S\r\nUsage:%s c/C target_addr\r\n", arg, arg);
}

int main(int argc, char **argv) {
	int sockfd, client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t cin_size;
	char buf[BUFFER_SIZE];
	int real_read, real_write;
	int flags;
	int ret;
	/* 参数检测 */
	if (argc <= 1) {
		Usage(argv[0]);
		exit(1);
	}
	/* 服务端 */
	if (strncasecmp(argv[1], "s", 1) == 0) {
		/* 创建流式套接字 */
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			perror("socket");
			exit(1);
		}
		/* 套接字绑定地址信息 */
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(PORT);
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		ret = bind(sockfd, (struct sockaddr*) &server_addr,
				sizeof(server_addr));
		if (ret < 0) {
			perror("bind");
			exit(1);
		}
		/* 设定监听队列最大长度 */
		ret = listen(sockfd, MAX_QUE_CONN_NUM);
		if (ret < 0) {
			perror("listen");
			exit(1);
		}
		/* 设定套接字非阻塞属性*/
		flags = fcntl(sockfd, F_GETFL);
		if (flags < 0 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
			perror("fcntl");
			exit(1);
		}
		/* 非阻塞方式等待客户端的连接 */
		cin_size = sizeof(client_addr);
		while (1) {
			client_fd = accept(sockfd, (struct sockaddr*) &client_addr,
					&cin_size);
			if (client_fd < 0) {
				if (errno == EAGAIN) {
					printf("Resource temporarily unavailable\r\n");
					sleep(1);
				} else {
					perror("accept");
					exit(1);
				}
			} else {
				break;
			}
		}
		printf("Find a client(%s)\r\n", inet_ntoa(client_addr.sin_addr));
		/* 循环读取客户端传来的数据 */
		while (1) {
			/* 接收客户端发送的数据 */
			memset(buf, 0, BUFFER_SIZE);
			real_read = recv(client_fd, buf, BUFFER_SIZE, 0);
			if (real_read < 0) {
				perror("recv");
				exit(1);
			} else if (real_read == 0) {
				printf("Peer has exit\r\n");
				break;
			} else {
				printf("Receive a message from %s:%s\r\n",
						inet_ntoa(client_addr.sin_addr), buf);
				if (strncmp(buf, "quit", 4) == 0) {
					break;
				}
			}
		}
		close(sockfd);
		close(client_fd);
	}
	/* 客户端 */
	else if (strncasecmp(argv[1], "c", 1) == 0) {
		if (argc <= 2) {
			Usage(argv[0]);
			exit(1);
		}
		/* 创建流式套接字 */
		client_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (client_fd < 0) {
			perror("socket");
			exit(1);
		}
		/* 向服务端发起连接请求 */
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(PORT);
		ret = inet_aton(argv[2], &server_addr.sin_addr);
		if (ret == 0) {
			perror("inet_aton");
			exit(1);
		}
		ret = connect(client_fd, (struct sockaddr*) &server_addr,
				sizeof(server_addr));
		if (ret < 0) {
			perror("connect");
			exit(1);
		}
		printf("Connect to server OK\r\n");
		/* 循环读取用户输入发送给服务器 */
		while (fgets(buf, BUFFER_SIZE, stdin)) {
			real_write = send(client_fd, buf, strlen(buf), 0);
			if (real_write < 0) {
				perror("send");
				exit(1);
			}
			if (strncmp(buf, "quit", 4) == 0) {
				break;
			}
		}
		close(client_fd);
	} else {
		Usage(argv[0]);
		exit(1);
	}
	return 0;
}

