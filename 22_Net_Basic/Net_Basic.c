/*
 * 	Net_Basic.c
 *
 *  Created on: 2016年11月6日
 *      Author: morris
 *  要求：
 *  	网络编程基础
 *  	用两种方法获取主机名与IP地址
 *	********************************************************************
 *  1. TCP实体所采用的基本协议是滑动窗口协议。当发送方传送一个数据报时，它将启动计时器。
 *  当该数据报到达目的地后，接收方的TCP实体向回发送一个数据报，其中包含有一个确认序号，
 *  表示希望收到的下一个数据报的顺序号。如果发送方的定时器在确认信息到达之前超时，那么
 *  发送方会重发该数据报
 *  2. UDP比TCP协议更高效，也能更好地解决实时性的问题
 *  3. Linux中的网络编程是通过socket接口进行的。socket是一种特殊的I/O接口，也是一种
 *  文件描述符。
 *  4. socket是一种常用的进程间通信机制，通过它不仅能实现本地机器上的进程之间的通信，
 *  而且通过网络能够在不同机器上的进程之间进行通信
 *  5. 常见的socket有3种类型
 *  	SOCK_STREAM：流式套接字提供可靠的、面向连接的通信流，使用TCP协议，
 *  	保证了数据传输的可靠性和顺序性
 *  	SOCK_DGRAM：数据报套接字定义了一种非可靠、面向无连接的服务，
 *  	数据通过相互独立的保温进行传输，是无序的，并且不保证是可靠的、无差错的，
 *  	使用数据报协议UDP
 *  	SOCK_RAW：原始套接字允许对底层协议，如IP或ICMP进行直接访问，
 *  	它功能强大但使用较为不便，主要用于一些协议的开发
 *  6. 通常服务器端在调用getaddrinfo之前，ai_flag设置为AI_PASSIVE，用于bind函数，
 *  主机名node通常会设置为NULL;客户端在调用getaddrinfo的时候，ai_flag一般不设置
 *  AI_PASSIVE，但是主机名noden和服务名service(端口)应该不为空
 *  ********************************************************************
 *	struct sockaddr//套接字地址通用结构体
 *	{
 *		unsigned short sa_family;//地址族
 *		char sa_data[14];//14字节的协议地址，包含该socket的IP地址和端口号
 *	};
 *	struct sockaddr_in				//in代表internet
 *	{
 *		short int sa_family;		//地址族
 *		unsigned short int sin_port;//端口号,2字节，网络字节序
 *		struct in_addr sin_addr;	//IP地址，4字节，网络字节序
 *		unsigned char sin_zero[8];	//填充0以保持与struct sockaddr同样大小
 *	};
 *	struct in_addr
 *	{
 *		uint32_t s_addr;//实质上是32位整型网络字节序
 *	}
 *	这两个数据类型是等效的,都用来保存socket地址信息，可以相互转化，其中
 *	sa_family字段可选的常见值：
 *		AF_INET：IPv4协议
 *		AF_INET6：IPv6协议
 *		AF_LOCAL：UNIX域协议
 *		AF_LINK：链路地址协议
 *		AF_KEY：密钥套接字
 *	数据存储优先顺序（大小端转化,本地字节序与网络字节序的转化）
 *	uint32_t htonl(uint32_t hostlong);本地32位数据转换成网络字节序
 *	uint16_t htons(uint16_t hostshort);本地16位数据转换成网络字节序
 *	uint32_t ntohl(uint32_t netlong);网络32位数据转换成本地字节序
 *	uint16_t ntohs(uint16_t netshort);网络16位数据转换成本地字节序
 *		hostlong:主机字节序的32bit数据,通常是IPv4地址
 *		hostshort:主机字节序的16bit数据，通常时端口号
 *		netlong:网络字节序的32bit数据
 *		netshort:网络字节序的16bit数据
 *		返回：
 *			成功：返回要转换的字节序
 *			出错:返回-1
 *	in_addr_t inet_addr(const char* cp);地址格式转换
 *		cp:点分十进制表示的地址的字符串
 *		返回：
 *			成功：返回32位整数格式的IP地址，网络字节序
 *			失败：返回-1
 *		局限：只能适用于IPv4，并且不能用于255.255.255.255的转换
 *	int inet_pton(int family,const char* strptr, void* addrptr);
 *	将点分十进制地址字符串转换为二进制地址
 *		family:
 *			AF_INET:IPv4协议
 *			AF_INET6:IPv6协议
 *		strptr:要转换的值（十进制地址字符串,点分形式或者以冒号分隔）
 *		addrptr:转换后的二进制地址,网络字节序
 *		返回：
 *			成功：返回1
 *			出错：返回0
 *	int inet_ntop(int family,void* addrptr, char* strptr,size_t len);
 *	将二进制地址转换为点分十进制地址字符串
 *		family:
 *			AF_INET:IPv4协议
 *			AF_INET6:IPv6协议
 *		addrptr:要转换的二进制地址
 *		strptr:转换后的十进制地址字符串
 *		len:转换后值的大小
 *		返回：
 *			成功：返回1
 *			出错：返回0
 *	名字地址转换
 *	struct hostent* gethostbyname(const char* hostname);
 *	通过主机名获得主机地址信息，域名解析
 *		hostname:主机域名或者IP名,如“www.google.com”或者“192.168.0.1”
 *		返回：
 *			成功：返回hostent类型指针
 *			出错：返回NULL
 *	struct hostent *gethostbyaddr(const void *addr,socklen_t len,
 int type);将IP地址转换为主机名
 *		struct hostent
 *		{
 *			char* h_name;		//正式主机名
 *			char** h_aliases;	//主机别名
 *			int h_addrtype;		//地址类型,取值可以是AF_INET或者AF_INET6
 *			int h_length;		//地址字节长度
 *			char** h_addr_list;	//指向IPv4(32位网络字节序整数)或
 *								//IPv6的地址指针数组
 *		}
 *	void endhostent(void);
 *		释放因调用gethostbyname函数而申请的hostent结构体空间
 *	void herror(const char* s);
 *		打印域名转换导致的出错信息
 *	int getaddrinfo(const char *node, const char *service,
 const struct addrinfo *hints,struct addrinfo **res);
 * 	IPv4和IPv6的地址和主机名之间相互转换
 *		node:网络地址或者网络主机名
 *		service:服务名或者十进制的端口号字符串
 *		hints:服务线索
 *		result:返回结果
 *		返回：
 *			成功:返回0
 *			出错：返回-1
 *	struct addrinfo
 *	{
 *		int ai_flags;//AI_PASSIVE该套接口用作被动打开
 *					//AI_CANONNAME通知getaddrinfo函数返回主机的名字
 *		int ai_family;//AF_INET:IPv4;
 *					//AF_INET6:IPv6;
 *					//AF_UNSPEC:IPv4或IPv6均可
 *		int ai_sockettype;//SOCK_STREAM:字节流套接字socket（TCP）
 *						//SOCK_DGRAM：数据报套接字socket（UDP）
 *		int ai_protocol;//IPPROTO_IP:IP协议;
 *						//IPPROTO_IPV4:IPv4协议;
 *						//IPPROTO_IPv6：IPv6协议;
 *						//IPPROTO_UDP：UDP协议;
 *						//IPPROTO_TCP：TCP协议
 *		size_t ai_addrlen;//地址字节长度
 *		char* ai_canonname;//主机名
 *		struct sockaddr* ai_addr;//sockaddr结构体指针
 *		struct addrinfo* ai_next;//下一个指针链表
 *	}
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAXNAMELEN			256

int main(int argc, char **argv) {
	struct addrinfo hints;
	struct addrinfo* res = NULL;
	char hostname[MAXNAMELEN];
	char addr_str[INET_ADDRSTRLEN];
	struct in_addr addr;
	struct hostent* host_entity;
	int ret;
	int i;
	/* 构造主机地址信息提示 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;	//通知getaddrinfo函数返回主机名字
	hints.ai_family = AF_UNSPEC;	//IPv4与IPv6均可
	hints.ai_socktype = SOCK_DGRAM;	//数据报套接字
	hints.ai_protocol = IPPROTO_UDP;	//UDP协议
	/* 获取主机名字符串 */
	if (gethostname(hostname, MAXNAMELEN) == -1) {
		perror("gethostname\r\n");
		exit(1);
	}
	/* 获取主机地址信息 */
	ret = getaddrinfo(hostname, NULL, &hints, &res);
	if (ret != 0) {
		perror("getaddrinfo\r\n");
		exit(1);
	} else {
		/* 获取32位二进制IP地址 */
		addr = ((struct sockaddr_in*) (res->ai_addr))->sin_addr;
		/* 将32位二进制IP地址转换为字符串 */
		inet_ntop(res->ai_family, &(addr.s_addr), addr_str, INET_ADDRSTRLEN);
		printf("Host name:%s\r\nIP address:%s\r\n", res->ai_canonname,
				addr_str);
	}
	/* 这里再使用另外一中方法 */
	printf("****************\r\n");
	/* 根据主机名获取地址信息,域名解析 */
	host_entity = gethostbyname(hostname);
	if (host_entity == NULL) {
		herror("gethostbyname");
		exit(1);
	}
	printf("Host name:%s,address list:\r\n", host_entity->h_name);
	for (i = 0; host_entity->h_addr_list[i]; i++) {
		printf("\t%s\r\n",
				inet_ntoa(*(struct in_addr*) (host_entity->h_addr_list[i])));
	}
	endhostent();
	return 0;
}
