/*
 * IO_Poll.c
 *
 *  Created on: 2016年10月23日
 *      Author: morris
 *  要求：
 *		使用poll函数实现IO多路复用。
 *		监视两个管道输入和标准输入，将两个管道读取的输入字符串写入到标准输出文件中
 *		创建管道命令：mknod in1 p
 *		向管道中输入数据：cat > in1
 *	********************************************************************
 *	1. 从select和poll函数返回时，内核会通知用户已经准备好的文件描述符数量、已经准备好
 *	的条件等。通过使用select和poll函数的返回结果就可以调用相应的I/O处理函数
 *	2. select函数效率低，poll机制比select机制效率更高，使用更加广泛
 *  ********************************************************************
 *	int select(int nfds, fd_set *readfds, fd_set *writefds,
 fd_set *exceptfds, struct timeval *timeout);
 *		nfds:需要监视的文件描述符的最大值加1
 *		readfds:由select()监视的读文件描述符集合
 *		writefds:由select()监视的写文件描述符集合
 *		exeptfds:由select()监视的异常处理文件描述符集合
 *		timeout:
 *			NULL：永远等待，直到捕捉到信号或者文件描述符已经装备好为止
 *			具体值：如果等待了timeout时间还没有检测到任何文件描述符准备好，就立即返回
 *			0：从不等待，测试所有指定的描述符并立即返回
 *		return：
 *			成功：返回准备好的文件描述符
 *			超时：返回0
 *			出错：返回-1,并设置errno
 *	struct timeval
 *	{
 * 		time_t         tv_sec;     //seconds
 *   	suseconds_t    tv_usec;    //microseconds
 * 	};
 *	select()文件描述符集合处理函数
 *		FD_ZERO(fd_set* set)清除一个文件描述符集
 *		FD_SET(int fd, fd_set* set)将一个文件描述符加入文件描述符集中
 *		FD_CLR(int fd, fd_set* set)将一个文件描述符从文件描述符集中清除
 *		FD_ISSET(int fd, fd_set* set)如果文件描述符fd为fd_set集合中的一个元素，
 *		则返回非零值,用于调用select后测试文件描述符集中的哪个文件描述符是否有变化
 *	int poll(struct pollfd* fds, int numfds, int timeout);
 *		fds:用于描述需要对哪些文件的哪种类型的操作进行监控
 *		numfds：需要监听的文件个数，即第一个参数所指向的数组中的元素数目
 *		timeout：表示poll阻塞的超时时间（毫秒），如果该值小于等于0,表示无限等待
 *		return：
 *			成功：返回大于0的值，表示事件发生的pollfd结构的个数
 *			超时:返回0
 *			出错：返回-1,并设置errno
 *		struct pollfd
 *			{
 *				int fd;//需要监听的文件描述符
 *				short events;//需要监听的事件类型
 *						//POLLIN文件中有数据可以读
 *						//POLLPRI文件中有紧急数据可以读
 *						//POLLOUT可以向文件写入数据
 *						//POLLERR文件中出现错误，只限于输出
 *						//POLLHUP与文件的连接被断开，只限于输出
 *						//POLLNVAL文件描述符不合法，即没有指向一个成功打开的文件
 *				short revents;//已经发生的事件
 *			}
 */

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FIFO1_STR			"in1"
#define FIFO2_STR			"in2"
#define BUFFER_SIZE			(512)	//缓冲区大小
#define IN_FILES			(3)		//多路复用输入文件数目
#define TIME_DELAY			(10000)	//超时时间60s

int main() {
	struct pollfd fds[IN_FILES];
	char buf[BUFFER_SIZE];
	int i, real_read;
	int ret;

	/* 1. 设置需要监视的文件描述符集合 */
	fds[0].fd = 0;
	/* 以非阻塞的方式打开管道文件1 */
	fds[1].fd = open(FIFO1_STR, O_RDONLY | O_NONBLOCK);
	if (fds[1].fd < 0) {
		perror("open");
		exit(1);
	}
	/* 以非阻塞的方式打开管道文件2 */
	fds[2].fd = open(FIFO2_STR, O_RDONLY | O_NONBLOCK);
	if (fds[2].fd < 0) {
		perror("open");
		exit(1);
	}
	/* 2. 设置需要监听的事件类型:有数据可读 */
	for (i = 0; i < IN_FILES; i++) {
		fds[i].events = POLLIN;
	}
	/* 3. 循环测试是否存在正在监听的文件描述符 */
	while (fds[0].events || fds[1].events || fds[2].events) {
		ret = poll(fds, IN_FILES, TIME_DELAY);
		if (ret < 0) {
			perror("Poll");
			exit(1);
		} else if (ret == 0) {
			printf("Poll Time-out\r\n");
			exit(1);
		} else {
			for (i = 0; i < IN_FILES; i++) {
				if (fds[i].revents) {	//判断哪个文件发生了事件
					memset(buf, 0, BUFFER_SIZE);
					real_read = read(fds[i].fd, buf, BUFFER_SIZE);
					if (real_read < 0) {
						if (errno != EAGAIN) {	//容忍非阻塞操作中的EAGAIN错误
							perror("read");
							exit(1);
						}
					} else if (real_read == 0) {
						close(fds[i].fd);
						fds[i].events = 0;	//取消对该文件的监听
					} else {
						if (i == 0) {	//标准输入上由数据输入
							if (buf[0] == 'q' || buf[0] == 'Q') {
								exit(0);
							}
						} else {
							buf[real_read] = '\0';
							printf("FIFO%d:%s\r\n", i, buf);
						}
					}
				}
			}
		}
	}

	return 0;
}

