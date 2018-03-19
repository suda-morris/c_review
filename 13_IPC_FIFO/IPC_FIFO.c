/*
 * 	IPC_FIFO.c
 *
 *  Created on: 2016年11月1日
 *      Author: morris
 *  要求：
 *		进程间通信之---有名管道
 *		一个终端写数据到管道中，另一个终端从管道中读取数据
 *	********************************************************************
 *  1. 有名管道是对无名管道的一种改进，其特点为：
 *  	a. 可以使互不相关的两个进程实现彼此间通信
 *  	b. 该管道可以通过路径名来指定，并且在文件系统中是可见的。
 *  	c. 在建立了管道之后，两个进程就可以把它当作普通文件来进行读写操作,在创建管道
 *  	成功后，就可以使用open、read和write这些函数了
 *  	d. 严格的遵循先进先出规则,它们不支持文件定位操作，如lseek等
 *  2. 有名管道的创建可以使用mkfifo，该函数类似于文件中的open操作，
 *  可以指定管道的路径和打开模式。用户还可以在命令行使用“mknod 管道名 p”来创建有名管道
 *  3. 打开有名管道，如果只有读端或者写端，那么open函数会阻塞，只有读写端都存在，
 *  才能打开成功。这个问题可以在调用open函数的时候增加O_NONBLOCK选项解决
 *  4. ‘有名’的含义：有一个对应的文件存放在文件系统中（大小始终为0）
 *  ********************************************************************
 *	int mkfifo(const char* filename, mode_t mode);创建有名管道
 *		filename:要创建的管道名字
 *		mode：文件的权限,可以用八进制数表示
 *		返回：
 *			成功：返回0
 *			出错：返回-1，并设置errno
 *	int access(const char *name, int type);测试文件的访问权限
 *		name：文件名字符串
 *		type：测试的类型
 *			R_OK：是否可读
 *			W_OK：是否可写
 *			X_OK：是否可执行
 *			F_OK：是否存在
 *		返回：
 *			成功：返回0
 *			出错：返回-1,并设置errno
 */

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MYFIFO_STR				"/tmp/myfifo"	//有名管道文件名
#define BUFFER_SIZE				512

/* 程序用法提示 */
void Usage(char* arg) {
	printf("Usage:%s w/W/r/R [string for write]\r\n", arg);
}

int main(int argc, char **argv) {
	char buf[BUFFER_SIZE];
	int fd;
	int real_write, real_read;
	/* 参数检查 */
	if (argc <= 1) {
		Usage(argv[0]);
		exit(1);
	}
	/* 向FIFO中写入数据 */
	if ((strncasecmp(argv[1], "W", 1) == 0)) {
		if (argc <= 2) {
			Usage(argv[0]);
			exit(1);
		}
		/* 丛输入命令行中提取要写如的数据 */
		sscanf(argv[2], "%s", buf);
		/* 以只写阻塞的方式打开有名管道 */
		fd = open(MYFIFO_STR, O_WRONLY);
		if (fd < 0) {
			perror("open");
			exit(1);
		}
		/* 将数据写入管道 */
		real_write = write(fd, buf, BUFFER_SIZE);
		if (real_write == -1) {
			if (errno != EAGAIN) {
				perror("write");
				exit(1);
			}
		} else if (real_write > 0) {
			printf("Write '%s' to FIFO\r\n", buf);
		}
	} else if ((strncasecmp(argv[1], "R", 1) == 0)) { //从FIFO中读取数据
		/* 判断管道文件是否已经存在 */
		if (access(MYFIFO_STR, F_OK) == -1) {
			/* 管道不存在，若尚未创建，则以相应的权限创建 */
			if (mkfifo(MYFIFO_STR, 0666) < 0) {
				perror("mkfifo");
				exit(1);
			}
		}
		/* 以只读阻塞的方式打开管道文件 */
		fd = open(MYFIFO_STR, O_RDONLY);
		if (fd < 0) {
			perror("open");
			exit(1);
		}
		/* 循环读取管道中的数据 */
		while (1) {
			memset(buf, 0, BUFFER_SIZE);
			real_read = read(fd, buf, BUFFER_SIZE);
			if (real_read == -1) {
				if (errno != EAGAIN) {
					perror("read");
					exit(1);
				}
			} else if (real_read > 0) {
				printf("Read '%s' from FIFO\r\n", buf);
				/* 如果收到quit字符串就退出 */
				if (strncmp(buf, "quit", 4) == 0) {
					break;
				}
			}
		}
	} else {
		exit(1);
	}
	/* 关闭管道文件 */
	close(fd);
	return 0;
}

