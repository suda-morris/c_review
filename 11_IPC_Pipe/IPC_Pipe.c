/*
 * 	IPC_Pipe.c
 *
 *  Created on: 2016年11月1日
 *      Author: morris
 * 	要求：
 *		进程间通信之---无名管道
 *		父进程读取终端数据，通过无名管道传递给子进程
 *	********************************************************************
 * 	1. 管道使Linux进程间通信的一种方式，它把一个程序的输出直接连接到另一个程序的输入。
 * 	2. Linux的管道主要包括两种：无名管道和有名管道
 * 	3. 无名管道是Linux中管道通信的一种原始方法，其特点为：
 * 		a. 只能用于具有亲缘关系的进程之间的通信（也就是父子进程或者兄弟进程之间）
 * 		b. 是一个半双工的通信模式，具有固定的读端和写端
 * 	4. 管道也可以被看成一种特殊的文件，对于它的读写也可以使用read和write等函数。
 * 	但是它不是普通的文件，并不属于其他任何文件系统，并且只存在内存中
 * 	5. 管道是基于文件描述符的通信方式，当一个管道建立时，它会创建两个文件描述符:
 * 	fd[0]和fd[1]，其中fd[0]固定用于读管道，fd[1]固定用于写管道
 * 	6. 只有在管道的读端存在时，向管道写入数据才有意义。否则向管道写入数据的进程将收到
 * 	内核传来的SIGPIPE信号（通常为Broken pipe(管道断裂)错误）
 * 	7. 向管道写入数据时，Linux将不保证写入的原子性，管道缓冲区一有空闲区域，写进程就会
 * 	试图向管道写入数据。如果读进程不读取管道缓冲区中的数据，那么写操作就会一直阻塞
 * 	8. 父子进程在运行时，它们的先后次序并不能保证。因此，为了保证父子进程已经关闭了
 * 	相应的文件描述符，可在两个进程中调用sleep函数
 * 	9. 无名管道的数据存放于内存中，管道中的数据一旦读取就不存在了
 * 	10. 无名的含义：不存在于文件系统中，存在内存中
 * 	********************************************************************
 * 	int pipe(int fd[2]);无名管道创建函数
 * 		fd[2]:管道的两个文件描述符，之后就可以直接操作这两个文件描述符
 * 		返回：
 * 			成功：返回0
 * 			出错：返回-1,并设置errno
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE		256

int main() {
	int fds[2];
	int ret;
	pid_t pid;
	char buf[BUFFER_SIZE];
	int real_read, real_write;

	/* 1. 在创建子进程前先创建好无名管道 */
	ret = pipe(fds);
	if (ret < 0) {
		perror("pipe");
		exit(1);
	}
	/* 2. 创建子进程 */
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	} else if (pid == 0) {	//子进程，负责读
		/* 关闭写描述符 */
		close(fds[1]);
		do {
			memset(buf, 0, BUFFER_SIZE);
			/* 阻塞读管道中数据 */
			real_read = read(fds[0], buf, BUFFER_SIZE);
			if (real_read > 0) {
				printf("Child read from pipe:%s\r\n", buf);
				/* 如果收到quit则退出 */
				if (strncmp(buf, "quit", 4) == 0) {
					break;
				}
			} else if (real_read == 0) {
				break;
			} else {
				perror("read");
				exit(1);
			}
		} while (real_read >= 0);
		close(fds[0]);	//关闭子进程读描述符
		exit(0);
	} else {	//父进程，负责写
		/* 关闭读描述符 */
		close(fds[0]);
		memset(buf, 0, BUFFER_SIZE);
		/* 3. 循环读取终端数据，并写入管道传给子进程 */
		while (fgets(buf, BUFFER_SIZE, stdin)) {
			real_write = write(fds[1], buf, strlen(buf));
			if (real_write > 0) {
				printf("Father wrote to pipe:%s\r\n", buf);
			}
			if (strncmp(buf, "quit", 4) == 0) {
				break;
			}
			memset(buf, 0, BUFFER_SIZE);
		}
		close(fds[1]);	//关闭父进程写描述符
		waitpid(pid, NULL, 0);	//收集子进程退出信息
		exit(0);
	}
	return 0;
}

