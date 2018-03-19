/*
 * FIFO_Select.c
 *
 *  Created on: 2016年11月15日
 *      Author: morris
 *  要求：
 *		使用select多路复用函数实现如下功能:
 *		监听控制终端和两路有名管道中的输入数据，将管道中的输入数据打印在终端
 *		当控制终端输入quit字符串，程序退出
 *	********************************************************************
 */

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#define FIFO1_STR		"in1"
#define FIFO2_STR		"in2"
#define BUFFER_SIZE		512
#define	IN_FILES		3
#define MAX(a,b)		((a>b)?(a):(b))
#define TIME_DELAY		60

int main(int argc, char **argv) {
	int ret;
	int fds[IN_FILES];
	int maxfd;
	fd_set inset, tmp_inset;
	int i;
	struct timeval tv;
	char buf[BUFFER_SIZE];
	int real_read;

	/* 1. 判断管道文件是否存在，不存在则创建 */
	if (access(FIFO1_STR, F_OK) == -1) {
		ret = mkfifo(FIFO1_STR, 0666);
		if (ret < 0) {
			perror("mkfifo");
			exit(1);
		}
	}
	if (access(FIFO2_STR, F_OK) == -1) {
		ret = mkfifo(FIFO2_STR, 0666);
		if (ret < 0) {
			perror("mkfifo");
			exit(1);
		}
	}
	/* 2. 以只读非阻塞的方式打开两个管道文件 */
	fds[0] = 0; //标准输入
	fds[1] = open(FIFO1_STR, O_RDONLY | O_NONBLOCK);
	if (fds[1] < 0) {
		perror("open");
		exit(1);
	}
	fds[2] = open(FIFO2_STR, O_RDONLY | O_NONBLOCK);
	if (fds[2] < 0) {
		perror("open");
		exit(1);
	}
	/* 3. 获取最大的文件描述符 */
	maxfd = MAX(MAX(fds[0],fds[1]), fds[2]);
	/* 4. 初始化读描述符集合 */
	FD_ZERO(&inset);
	for (i = 0; i < IN_FILES; i++) {
		FD_SET(fds[i], &inset);
	}
	/* 5. 循环测试该文件描述符是否准备就绪，并调用select多路复用函数 */
	while (FD_ISSET(fds[0], &inset) || FD_ISSET(fds[1], &inset)
			|| FD_ISSET(fds[1], &inset)) {
		/* 文件描述符集合的备份，以免每次都要进行初始化 */
		tmp_inset = inset;
		/* 设置延迟时间 */
		tv.tv_sec = TIME_DELAY;
		tv.tv_usec = 0;
		ret = select(maxfd + 1, &tmp_inset, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select");
			exit(1);
		} else if (ret == 0) {	//超时
			printf("Time-out\r\n");
			exit(1);
		} else {
			/* 判断哪个文件描述符准备就绪 */
			for (i = 0; i < IN_FILES; i++) {
				if (FD_ISSET(fds[i], &tmp_inset)) {
					memset(buf, 0, BUFFER_SIZE);
					/* 读数据 */
					real_read = read(fds[i], buf, BUFFER_SIZE);
					if (real_read < 0 && errno != EAGAIN) {
						perror("read");
						exit(1);
					} else if (ret == 0) {
						/* 管道已被关闭 */
						close(fds[i]);
						FD_CLR(fds[i], &inset);
					} else {
						/* 如果是主程序的控制终端 */
						if (i == 0) {
							if (strncasecmp(buf, "quit", 4) == 0) {
								exit(0);
							}
						} else {
							/* 显示管道输入字符串 */
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
