/*
 * 	Net_TCP.c
 *
 *  Created on: 2016年11月7日
 *      Author: morris
 *  要求：
 *  	TCP编程---服务器+客户端
 *	********************************************************************
 *	1. 在服务器端，要建立TCP连接需要经过如下步骤
 *		a. 通过socket创建一个流式套接字
 *		b. 通过bind将套接字与服务器地址绑定
 *		c. 通过listen设定监听的端口数
 *		d. 通过accept等待接收客户端的连接请求
 *		e. 接收到连接请求后，使用read/write或者recv/send函数，通过从accept返回的
 *		客户端套接字与客户端进行通讯
 *	2. 在客户端，要与服务器建立TCP连接
 *		a. 通过socket创建一个流式套接字
 *		b. 通过connect向服务器地址发起连接请求
 *		c. 连接成功后使用read/write或者recv/send函数，通过之前的套接字与服务端通信
 *		d. 使用close关闭套接字，断开TCP连接
 *	3. 服务端套接字绑定地址信息的时候指定INADDR_ANY的好处？
 *		a. 这样，服务端程序就可以运行在任意IP地址的机器上(到处运行)
 *		b. 当运行在某机器上后，该机器上不同网口传送过来的数据，只要端口号，网络类型与
 *		服务端程序设定的一致，就能够被服务端程序接收处理（跨网口）
 *	********************************************************************
 *  int socket(int domain, int type, int protocol);创建套接字
 *  	domain:指定通信协议的协议族
 *  		AF_INET:IPv4协议
 *  		AF_INET6:IPv6协议
 *  		AF_LOCOL:UNIX域协议,本地通信
 *  		AF_ROUTE:路由套接字
 *  		AF_KEY:密钥套接字
 *  	type:指定套接字类型
 *  		SOCK_STREAM：字节流套接字
 *  		SOCK_DGRAM:数据报套接字
 *  		SOCK_RAW:原始套接字
 *  	protocol:指定应用程序使用的通信协议
 *  		0（原始套接字除外，因为TCP、UDP通信可以通过family域唯一指定，
 *  		原始套接字需要通过protocol域来加以区分IP、ICMP等等）
 *  	返回：
 *  		成功：返回非负套接字描述符
 *  		出错：返回-1,并设置errno
 *  int bind(int sockfd,struct sockaddr* my_addr,int addrlen);
 *  将套接字绑定到一个已知的地址上
 *  	sockfd:待绑定的套接字描述符
 *  	my_addr:待绑定的地址结构
 *  	addrlen:地址长度（my_addr的长度）
 *  	返回：
 *  		成功：返回0
 *  		出错：返回-1,并设置errno
 *  int listen(int sockfd,int backlog)把主动套接字变成被动套接字，
 *  并设置监听队列的最大长度
 *  	sockfd:套接字描述符
 *  	backlog:同时允许几路客户端和服务器进行正在连接的过程（正在三次握手），一般填5
 *  	内核中服务器的套接字fd会维护2个链表：
 *  			1. 正在三次握手的客户端链表（数量=2×backlog+1）
 *  			2. 已经建立好连接的客户端链表（已经完成3次握手分配好了newfd）
 *  	返回：
 *  		成功：返回0
 *  		出错：返回-1，并设置errno
 *  int accept(int sockfd,struct sockaddr* addr,socklen_t* addrlen);
 *  阻塞等待客户端连接请求
 *		sockfd:经过前面socket()创建并通过bind()、listen()设置过的套接字描述符
 *		addr:客户端地址信息
 *		addrlen:客户端地址长度,即addr参数的大小
 *		返回：
 *			成功：返回已经建立好连接的新的非负套接字
 *			出错：返回-1，并设置errno
 *	int connect(int sockfd,struct sockaddr* serv_addr,int addrlen);客户端
 *	向服务端发起连接请求
 *		sockfd:连接至服务端的套接字描述符
 *		serv_addr:指定的服务器端地址
 *		addrlen:地址长度，即serve_addr参数的大小
 *		返回：
 *			成功：返回0
 *			出错：返回-1，并设置errno
 *	int send(int sockfd,const void* msg,int len,int flags);发送通过远端主机
 *	指定的套接字数据
 *		sockfd:连接至远端主机的套接字描述符
 *		msg：指向要发送数据的指针
 *		len：数据长度
 *		flags：操作标志，一般为0。
 *			MSG_DONTWAIT：非阻塞发送
 *			MSG_NOSIGNAL:不会被SIGPIPE信号终端
 *		返回：
 *			成功：返回实际发送的字节数
 *			出错：返回-1,并设置errno
 *	int recv(int sockfd,void* buf,int len,unsigned int flags);接收远端主机
 *	指定套接字传来的数据
 *		sockfd:连接至远端主机的套接字描述符
 *		buf:接收数据的缓冲区
 *		len:缓冲区长度
 *		flags:操作标志，一般为0
 *			MSG_PEEK:返回的数据不会在系统内删除
 *			MSG_WAITALL:强迫接收到len大小的数据后才返回
 *			MSG_NOSIGNAL:不会被SIGPIPE信号中断
 *		返回：
 *			成功：返回实际接收到的字节数
 *			出错：返回-1，并设置errno
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT				4321
#define BUFFER_SIZE			1024
#define MAX_QUE_CONN_NM		5

/* 程序使用说明 */
void Usage(char* arg) {
	printf("Usage:%s s/S\r\nUsage:%s c/C ipaddr content\r\n", arg, arg);
}

int main(int argc, char **argv) {
	int sockfd, client_fd;
	struct sockaddr_in server_sockaddr, client_sockaddr;
	char buf[BUFFER_SIZE];
	int cin_size, recv_bytes, send_bytes;
	struct hostent* host;
	char ipv4_addr[16];
	int ret;
	/* 参数检查 */
	if (argc <= 1) {
		Usage(argv[0]);
		exit(1);
	}
	/* 服务端 */
	if ((strncasecmp(argv[1], "s", 1) == 0)) {
		/* 1. 创建流式套接字 */
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == -1) {
			perror("socket");
			exit(1);
		}
		printf("Socket ID=%d\r\n", sockfd);
		/* 2. 构造服务器端欲绑定的地址信息 */
		memset(&server_sockaddr, 0, sizeof(server_sockaddr));
		server_sockaddr.sin_family = AF_INET; //协议族
		server_sockaddr.sin_port = htons(PORT); //端口号
		server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		/* 3. 绑定socket与地址，端口号 */
		ret = bind(sockfd, (struct sockaddr*) (&server_sockaddr),
				sizeof(server_sockaddr));
		if (ret == -1) {
			perror("bind");
			exit(1);
		}
		printf("Bind success\r\n");
		/* 4. 创建未处理请求的队列，监听来自客户端的请求 */
		ret = listen(sockfd, MAX_QUE_CONN_NM);
		if (ret == -1) {
			perror("listen");
			exit(1);
		}
		printf("Listening...\r\n");
		/* 5. 等待客戶端连接 */
		cin_size = sizeof(client_sockaddr);
		client_fd = accept(sockfd, (struct sockaddr*) (&client_sockaddr),
				(socklen_t *) &cin_size);
		if (client_fd == -1) {
			perror("accept");
			exit(1);
		}
		/* 获取连接方的IP地址 */
		if (!inet_ntop(AF_INET, (void*) &client_sockaddr.sin_addr, ipv4_addr,
				sizeof(ipv4_addr))) {
			perror("inet_ntop");
			exit(1);
		}
		printf("Accept a client(%s)...\r\n", ipv4_addr);
		/* 6. 接收客户端发送的数据 */
		memset(buf, 0, BUFFER_SIZE);
		recv_bytes = recv(client_fd, buf, BUFFER_SIZE, 0);
		if (recv_bytes == -1) {
			perror("recv");
			exit(1);
		} else if (recv_bytes == 0) {	//对方已经关闭
			printf("Peer closed\r\n");
		} else {
			printf("Server Received a message :%s\r\n", buf);
		}
		/* 7. 关闭套接字描述符 */
		close(sockfd);
		close(client_fd);
	}
	/* 客户端 */
	else if ((strncasecmp(argv[1], "c", 1) == 0)) {
		if (argc <= 3) {
			Usage(argv[0]);
			exit(1);
		}
		/* 地址解析 */
		host = gethostbyname(argv[2]);
		if (host == NULL) {
			perror("gethostbyname");
			exit(1);
		}
		/* 准备要发送的数据 */
		memset(buf, 0, BUFFER_SIZE);
		sprintf(buf, "%s\r\n", argv[3]);
		/* 1. 创建流式套接字 */
		client_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (client_fd == -1) {
			perror("socket");
			exit(1);
		}
		/* 2. 向指定服务端发起链接请球 */
		memset(&server_sockaddr, 0, sizeof(server_sockaddr));
		server_sockaddr.sin_family = AF_INET;
		server_sockaddr.sin_port = htons(PORT);
		server_sockaddr.sin_addr = *((struct in_addr*) (host->h_addr_list[0]));
		ret = connect(client_fd, (struct sockaddr*) &server_sockaddr,
				sizeof(server_sockaddr));
		if (ret == -1) {
			perror("connect");
			exit(1);
		}
		printf("connect to server OK\r\n");
		/* 3. 向服务端发送数据 */
		send_bytes = send(client_fd, buf, strlen(buf), 0);
		if (send_bytes == -1) {
			perror("send");
			exit(1);
		}
		/* 4. 关闭套接字描述符 */
		close(client_fd);
	}
	return 0;
}

