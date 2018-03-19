/*
 * 	Net_Thread.c
 *
 *  Created on: 2016年11月18日
 *      Author: morris
 *  要求：
 *  	网络编程之---多线程并发服务器
 *  	对每个连接来的客户端创建一个线程，单独与其进行通信
 *	********************************************************************
 *	1. 所谓并发服务器，就是指能够同时处理多个客户请求的服务器
 *	2. 实现并发服务器，主要有两种：
 *		a. 并发连接服务器：在accept函数监听到连接请求后，产生子进程/线程处理用户请求
 *		b. 单进程线程并发服务器：通过select函数，用多路复用I/O实现处理多个客户的连接
 *	********************************************************************
 */

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define PORT				5004	//端口号
#define MAX_QUE_CONN_NUM	5		//最大同时支持连接服务器的数量
#define BUFFER_SIZE			1024	//数据缓冲区大小
#define MAX(a,b)			((a>b)?(a):(b))

char buf[BUFFER_SIZE];
int real_read, real_write;

/* 程序使用说明 */
void Usage(char* arg) {
	printf("Usage:%s s/S\r\nUsage:%s c/C target_addr\r\n", arg, arg);
}

/* 通信子线程 */
void* thrd_accept(void* arg) {
	int* parg = (int*) arg;
	/* 一定要用一个局部变量保存传递过来的数据，直接操作指针arg达不到并发服务器的效果 */
	int client_fd = *parg;
	/* 循环读取客户端输入数据 */
	while (1) {
		memset(buf, 0, BUFFER_SIZE);
		real_read = recv(client_fd, buf, BUFFER_SIZE, 0);
		if (real_read < 0) {
			perror("recv");
			break;
		} else if (real_read == 0) {
			printf("Client %d has exited", client_fd);
			break;
		} else {
			printf("Receive from client %d:%s\r\n", client_fd, buf);
			if (strncmp(buf, "quit", 4) == 0) {
				printf("client %d(socket) has exited\r\n", client_fd);
				break;
			}
			strcat(buf, "OK");
			real_write = send(client_fd, buf, strlen(buf), 0);
			if (real_write < 0) {
				perror("send");
				break;
			}
		}

	}
	close(client_fd);
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
	int server_fd, client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t cin_size;
	fd_set inset, tmp_inset;
	pthread_t trd_id;
	int max_fd = -1;
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
		/* 允许重复使用本地地址与套接字进行绑定 */
		int b_reuse = 1;
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &b_reuse,
				sizeof(b_reuse));
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
		/* 对每个连接来的客户端都将创建一个线程 */
		while (1) {
			cin_size = sizeof(client_addr);
			client_fd = accept(server_fd, (struct sockaddr*) &client_addr,
					&cin_size);
			if (client_fd < 0) {
				perror("accept");
				exit(1);
			}
			printf("New client %d(socket)\r\n", client_fd);
			/* 创建一个新线程，将套接字描述符作为参数传入 */
			ret = pthread_create(&trd_id, NULL, thrd_accept,
					(void*) &client_fd);
			if (ret != 0) {
				perror("pthread_create");
				exit(ret);
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
		/* 构造需要监听的输入描述符集合 */
		FD_ZERO(&inset);
		FD_SET(0, &inset); //标准输入
		FD_SET(client_fd, &inset); //接收来自服务器的输入
		max_fd = MAX(0, client_fd);
		while (FD_ISSET(0,&inset) || FD_ISSET(client_fd, &inset)) {
			tmp_inset = inset;
			ret = select(max_fd + 1, &tmp_inset, NULL, NULL, NULL);
			if (ret < 0) {
				perror("select");
				exit(1);
			}
			/* 终端输入 */
			if (FD_ISSET(0, &tmp_inset)) {
				memset(buf, 0, BUFFER_SIZE);
				if (fgets(buf, BUFFER_SIZE, stdin)) {
					/* 终端输入的数据发送给服务器 */
					real_write = send(client_fd, buf, strlen(buf), 0);
					if (real_write < 0) {
						perror("send");
					}
					if (strncmp(buf, "quit", 4) == 0) {
						break;
					}
				}
			}
			/* 服务器响应 */
			if (FD_ISSET(client_fd, &tmp_inset)) {
				memset(buf, 0, BUFFER_SIZE);
				real_read = recv(client_fd, buf, BUFFER_SIZE, 0);
				if (real_read < 0) {
					perror("recv");
				} else if (real_read == 0) {
					printf("server exited\r\n");
					FD_CLR(client_fd, &inset);
					break;
				} else {
					printf("From server:%s\r\n", buf);
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

