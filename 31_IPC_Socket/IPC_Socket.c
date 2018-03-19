/*
 * 	IPC_Socket.c
 *
 *  Created on: 2016年11月20日
 *      Author: morris
 *  要求：
 *  	网络编程之---UNIX域套接字编程（进程间通信的另一种方式）
 *  	利用UNIX域套接字实现进程间的通信
 *	********************************************************************
 *	1. UNIX域套接字可用于本地进程间通信
 *	2. 创建套接字时使用本地协议AF_LOCAL
 *	3. 总结Linux中进程间通信：
 *		a. 进程间的数据共享：管道、消息队列、共享内存UNIX域套接字
 *		b. 异步通信：信号
 *		c. 同步与互斥：信号量
 *	4. 从易用性来说：消息队列>UNIX域套接字>管道>共享内存
 *	5. 从效率上来说：共享内存>UNIX域套接字>管道>消息队列
 *	********************************************************************
 *	UNIX域套接字的本地地址结构
 *	struct sockaddr_un
 *	{
 *		sa_family_t sun_family;	//填AF_UNIX/AF_LOCAL
 *		char sun_path[108];	//套接字文件的绝对路径，必须事先不存在,该文件在内存中
 *	}
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define UNIX_DOMAIN_FILE		"/tmp/unix_domain"
#define BUFFER_SIZE				1024
#define MAX_CONN_NUM			5

void Usage(char* arg) {
	printf("Usage:%s s/S\r\nUsage:%s c/C\r\n", arg, arg);
}

int main(int argc, char **argv) {
	int server_fd, client_fd;
	struct sockaddr_un sun;
	int real_read, real_write;
	char buf[BUFFER_SIZE];
	int ret;
	/* 参数检查 */
	if (argc <= 1) {
		Usage(argv[0]);
		exit(1);
	}
	/* 服务端 */
	if (strncasecmp(argv[1], "s", 1) == 0) {
		/* 创建流式套接字 */
		server_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
		if (server_fd < 0) {
			perror("socket");
			exit(1);
		}
		/* 填充UNIX域套接字本地地址 */
		memset(&sun, 0, sizeof(sun));
		sun.sun_family = AF_LOCAL;
		/* 服务器端检查UNIX域套接字文件是否存在，若存在则删除 */
		ret = access(UNIX_DOMAIN_FILE, F_OK);
		if (ret == 0) {
			unlink(UNIX_DOMAIN_FILE);
		}
		strncpy(sun.sun_path, UNIX_DOMAIN_FILE, strlen(UNIX_DOMAIN_FILE));
		/* 绑定UNIX域套接字 */
		ret = bind(server_fd, (struct sockaddr*) &sun, sizeof(sun));
		if (ret < 0) {
			perror("bind");
			exit(1);
		}
		/* 设置允许同时发起连接的最大值 */
		ret = listen(server_fd, MAX_CONN_NUM);
		if (ret < 0) {
			perror("listen");
			exit(1);
		}
		client_fd = accept(server_fd, NULL, NULL);
		if (client_fd < 0) {
			perror("accept");
			exit(1);
		}
		printf("A new client found:%d\r\n", client_fd);
		/* 循环读取客户端发送的数据 */
		while (1) {
			memset(buf, 0, BUFFER_SIZE);
			real_read = recv(client_fd, buf, BUFFER_SIZE, 0);
			if (real_read < 0) {
				perror("recv");
				exit(1);
			} else if (real_read == 0) {
				printf("Peer exited\r\n");
				close(client_fd);
				exit(1);
			} else {
				printf("Receive a message:%s\r\n", buf);
				if (strncmp(buf, "quit", 4) == 0) {
					break;
				}
			}
		}
		close(server_fd);
	}
	/* 客户端 */
	else if (strncasecmp(argv[1], "c", 1) == 0) {
		/* 创建流式套接字 */
		client_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
		if (client_fd < 0) {
			perror("socket");
			exit(1);
		}
		/* 填充服务器地址结构 */
		memset(&sun, 0, sizeof(sun));
		sun.sun_family = AF_LOCAL;
		/* 客户端要确保UNIX_DOMAIN_FILE已经被服务端创建,不存在则退出 */
		if (access(UNIX_DOMAIN_FILE, F_OK | W_OK) < 0) {
			printf("No local server\r\n");
			exit(1);
		}
		strncpy(sun.sun_path, UNIX_DOMAIN_FILE, strlen(UNIX_DOMAIN_FILE));
		/* 向服务端发起连接请求 */
		ret = connect(client_fd, (struct sockaddr*) &sun, sizeof(sun));
		if (ret < 0) {
			perror("connect");
			exit(1);
		}
		/* 循环读取用户输入，发送给服务端 */
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

