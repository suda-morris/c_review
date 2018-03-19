/*
 * 	Net_Async.c
 *
 *  Created on: 2016年11月18日
 *      Author: morris
 *  要求：
 *  	网络编程之---异步IO
 *  	实现基于异步I/O方式的套接字通信
 *	********************************************************************
 *	1. 内核通过使用异步IO，在某个进程需要处理的事件发生（如接收到新的连接请求）时，向该
 *	进程发送一个SIGIO信号。这样，应用程序不需要不停地等待某些事件的发生，而可以往下运行
 *	2. 使用fcntl函数就可以实现高效率的异步I/O方法。首先必须使用fcntl函数的F_SETDOWN
 *	命令，使套接字归属当前进程，这样内核能够判断应该向哪个进程发送信号。
 *	3. 接下来，使用fcntl函数的F_SETFL命令将套接字的状态标志位设置为异步通知方式（使用
 *	O_ASYNC参数）
 *	********************************************************************
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT			5002
#define BUFFER_SIZE		1024
#define MAX_CONN_NUM	5

struct sockaddr_in client_addr;
socklen_t addr_len;
int server_fd, client_fd;
char buf[BUFFER_SIZE];
int real_read;

/* 程序使用介绍 */
void Usage(char* arg) {
	printf("Usage:%s s/S\r\nUsage:%s c/C target_addr\r\n", arg, arg);
}

/* 异步信号处理函数，处理新的套接字的连接和数据 */
void accept_async(int sig_num) {
	printf("get SIGIO\r\n");
	/* 接收客户端的连接请求 */
	addr_len = sizeof(client_addr);
	client_fd = accept(server_fd, (struct sockaddr*) &client_addr, &addr_len);
	if (client_fd < 0) {
		perror("accept");
		exit(1);
	}
	printf("Accept a client(%s)\r\n", inet_ntoa(client_addr.sin_addr));
	/* 接收客户端的输入数据 */
	memset(buf, 0, BUFFER_SIZE);
	real_read = recv(client_fd, buf, BUFFER_SIZE, 0);
	if (real_read < 0) {
		perror("recv");
		exit(1);
	} else if (real_read == 0) {
		printf("Peer exited\r\n");
		exit(1);
	} else {
		printf("Receive s message from %s:%s", inet_ntoa(client_addr.sin_addr),
				buf);
		if (strncmp(buf, "quit", 4) == 0) {
			exit(0);
		}
	}
	close(client_fd);
}

int main(int argc, char **argv) {
	struct sockaddr_in server_addr;
	int real_write;
	int flags;
	int ret;
	/* 参数检查 */
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
		ret = listen(server_fd, MAX_CONN_NUM);
		if (ret < 0) {
			perror("listen");
			exit(1);
		}
		/* 将套接字设置为异步工作方式 */
		/* 绑定套接字与当前进程 */
		ret = fcntl(server_fd, F_SETOWN, getpid());
		if (ret < 0) {
			perror("fcntl(F_SETOWN)");
			exit(1);
		}
		/* 获得套接字的状态标志位 */
		flags = fcntl(server_fd, F_GETFL);
		if (flags < 0) {
			perror("fcntl(F_GETFL)");
			exit(1);
		}
		/* 设置成异步访问模式 */
		ret = fcntl(server_fd, F_SETFL, flags | O_ASYNC);
		if (ret < 0) {
			perror("fcntl(F_SETFL)");
			exit(1);
		}
		/* 注册SIGIO信号处理函数 */
		signal(SIGIO, accept_async);
		/* 处理别的任务 */
		while (1) {
			sleep(1);
			printf("Server is working...\r\n");
			/* 接收到quit字符串就退出 */
			if (strncmp(buf, "quit", 4) == 0) {
				break;
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
		/* 向服务端发出连接请求 */
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
		/* 向服务器发送数据 */
		if (fgets(buf, BUFFER_SIZE, stdin)) {
			real_write = send(client_fd, buf, strlen(buf), 0);
			if (real_write < 0) {
				perror("send");
				exit(1);
			}
		}
		close(client_fd);
	} else {
		Usage(argv[0]);
		exit(1);
	}
	return 0;
}

