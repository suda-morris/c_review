/*
 * 	Net_Select.c
 *
 *  Created on: 2016年11月18日
 *      Author: morris
 * 	要求：
 *  	网络编程之---IO多路复用(Select)
 *  	使用select多路复用实现单进程单线程并发服务器
 *	********************************************************************
 *	1. 应用程序中同时处理多路输入流，若采用阻塞模式，将得不到预期的目的
 *	2. 若采用非阻塞模式，对多个输入进行轮询，太浪费CPU时间
 *	3. 若设置多个进程，分别处理一条数据通路，将产生进程间的同步与通信问题，复杂
 *	4. 比较好的方法是使用I/O多路复用
 *	********************************************************************
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define PORT				5003	//端口号
#define MAX_QUE_CONN_NUM	5		//最大同时支持连接服务器的数量
#define BUFFER_SIZE			1024	//数据缓冲区大小
#define TIME_OUT			10		//select超时时间
#define MAX_SOCK_FD			FD_SETSIZE	//fd_set中最大元素数量
#define SERVER_IDENT		"\t(From Server)"
#define MAX(a,b)			((a>b)?(a):(b))

/* 程序使用说明 */
void Usage(char* arg) {
	printf("Usage:%s s/S\r\nUsage:%s c/C target_addr\r\n", arg, arg);
}

int main(int argc, char **argv) {
	int server_fd, client_fd;
	int maxfd;
	struct sockaddr_in server_addr, client_addr;
	fd_set inset, tmp_inset;
	socklen_t cin_size;
	struct timeval tv;
	char buf[BUFFER_SIZE];
	int real_read, real_write;
	int i;
	int ret;
	/*参数检查*/
	if (argc <= 1) {
		Usage(argv[0]);
		exit(1);
	}
	/* 服务端 */
	if (strncasecmp(argv[1], "s", 1) == 0) {
		/* 创建流式套接字 */
		server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd < 0) {
			perror("socket");
			exit(1);
		}
		/* 套接字绑定地址信息 */
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(PORT);
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		ret = bind(server_fd, (struct sockaddr*) &server_addr,
				sizeof(server_addr));
		if (ret < 0) {
			perror("bind");
			exit(1);
		}
		/* 设置监听队列最大长度 */
		ret = listen(server_fd, MAX_QUE_CONN_NUM);
		if (ret < 0) {
			perror("listen");
			exit(1);
		}
		printf("Listening...\r\n");
		/* 构造读文件描述符集合 */
		FD_ZERO(&inset);
		FD_SET(0, &inset);	//标准经盘输入
		FD_SET(server_fd, &inset);	//负责等待客户端连接
		/* 循环等待事件发生 */
		while (1) {
			/* 文件描述符集合备份，这样可以避免每次都进行初始化 */
			tmp_inset = inset;
			tv.tv_sec = TIME_OUT;
			tv.tv_usec = 0;
			/* select多路复用 */
			ret = select(MAX_SOCK_FD, &tmp_inset, NULL, NULL, &tv);
			if (ret < 0) {
				perror("select");
				exit(1);
			} else if (ret == 0) {
				printf("Time-out\r\n");
				break;
			}
			/* 轮询每个描述符 */
			for (i = 0; i < MAX_SOCK_FD; i++) {
				if (FD_ISSET(i, &tmp_inset)) {
					/* 有新的连接请求到来 */
					if (i == server_fd) {
						cin_size = sizeof(client_addr);
						/* 接收客户端的连接请求 */
						client_fd = accept(i, (struct sockaddr*) &client_addr,
								&cin_size);
						if (client_fd < 0) {
							perror("accept");
							exit(1);
						}
						/* 将新连接的客户端套接字加入观察列表中 */
						FD_SET(client_fd, &inset);
						printf("New connection from %d(socket)\r\n", client_fd);
					}
					/* 有新的数据发送过来 */
					else {
						memset(buf, 0, BUFFER_SIZE);
						real_read = recv(i, buf, BUFFER_SIZE, 0);
						if (real_read < 0) {
							perror("recv");
						} else if (real_read == 0) {
							printf("Client %d(socket) has left\r\n", i);
							close(i);
							FD_CLR(i, &inset);
						} else {
							printf("Receive a message from %d(socket):%s\r\n",
									i, buf);
							strncat(buf, SERVER_IDENT, sizeof(SERVER_IDENT));
							real_write = send(i, buf, strlen(buf), 0);
							if (real_write < 0) {
								perror("send");
							}
						}
					}
				}
			}
		}
		close(server_fd);
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
		/* 向服务器发起连接连接请求 */
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
		/* 构造读文件描述符集合 */
		FD_ZERO(&inset);
		FD_SET(0, &inset);	//标准输入
		FD_SET(client_fd, &inset);	//客户端网络套接字
		maxfd = MAX(0, client_fd);
		while (FD_ISSET(0,&inset) || FD_ISSET(client_fd, &inset)) {
			/* 备份文件描述符集合 */
			tmp_inset = inset;
			tv.tv_sec = TIME_OUT;
			tv.tv_usec = 0;
			/* select多路复用 */
			ret = select(maxfd + 1, &tmp_inset, NULL, NULL, &tv);
			if (ret < 0) {
				perror("select");
				exit(1);
			} else if (ret == 0) {
				printf("Time-out\r\n");
				break;
			}
			if (FD_ISSET(0, &tmp_inset)) {	//标准键盘有输入
				memset(buf, 0, BUFFER_SIZE);
				if (fgets(buf, BUFFER_SIZE, stdin)) {
					real_write = send(client_fd, buf, strlen(buf), 0);
					if (real_write < 0) {
						perror("send");
					}
					if (strncmp(buf, "quit", 4) == 0) {
						break;
					}
				}
			}
			if (FD_ISSET(client_fd, &tmp_inset)) {	//服务端发送了数据过来
				memset(buf, 0, BUFFER_SIZE);
				real_read = recv(client_fd, buf, BUFFER_SIZE, 0);
				if (real_read < 0) {
					perror("recv");
				} else if (real_read == 0) {
					printf("server exited\r\n");
					FD_CLR(client_fd, &inset);
					break;
				} else {
					printf("server return: %s\r\n", buf);
				}
			}
		}
		close(client_fd);
	} else {
		Usage(argv[0]);
		exit(1);
	}
	return 0;
}

