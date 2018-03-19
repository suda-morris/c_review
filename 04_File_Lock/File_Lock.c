/*
 * main.c
 *
 *  Created on: 2016年10月23日
 *      Author: morris
 *  要求：
 *		文件读取锁功能测试
 *	********************************************************************
 *	1. 当多个用户共同使用、操作一个文件的时候，Linux通常采用的方法是给文件上锁，
 *	避免共享的资源产生竞争的状态。
 *	2. 文件锁包括建议性锁和强制性锁。
 *	3. 建议性锁要求每个上锁文件的进程都要检查是否有锁存在，并且尊重已有的锁。
 *	在一般情况下，内核和系统都不使用建议性锁。
 *	4. 强制性锁是由内核执行的锁，当一个文件被上锁进行写入操作时，内核将阻止其他任何文件
 *	对其进行读写操作。
 *	5. 记录锁分为读取锁（共享锁）和写入锁（排斥锁）。
 *	6. 共享锁能够使多个进程都在文件的同一部分建立读取锁。
 *	在任何时刻只能有一个进程在文件的某个部分建立写入锁
 *	********************************************************************
 *	int fcntl(int fd, int cmd, struct flock* lock);
 *		fd：文件描述符
 *		cmd：操作命令
 *			F_DUPFD：复制文件描述符
 *			F_GETFD：获得fd的close-on-exec标志，若标志未设置，
 *			则文件经过exec()函数之后仍然保持打开状态
 *			F_SETFD：设置close-on-exec标志，该标志由参数arg的FD_CLOEXEC位决定
 *			F_GETFL：得到open设置的标志
 *			F_SETFL：改变open设置的标志
 *			F_GETLK：判断是否可以进行lock结构体所描述的锁操作，
 *			若可以进行，则flock结构体的l_type会被设置为F_UNLCK，其他域不变；
 *			若不可以进行，则l_pid被设置为拥有文件锁的进程号，其他域不变
 *			F_SETLK：设置lock描述的文件锁
 *			F_SETLKW：这是F_SETLK的阻塞版本，在无法获取锁的时候，会进入睡眠状态。
 *			如果可以获取锁或者捕捉到信号则会返回
 *		lock：设置记录锁的具体状态，
 *		为了加锁整个文件，通常l_start设置为0,l_whence设置为SEEK_SET，l_len设置为0
 *		return：
 *			成功：返回0
 *			出错：返回-1，并设置errno
 *		struct flock
 *		{
 *			short l_type;	//F_RDLCK读取锁，F_WRLCK写入锁，F_UNLCK解锁
 *			off_t l_start;	//加锁区域在文件中的相对位移量（字节），
 *							//与l_whence一同决定加锁区域的起始位置
 *			short l_whence;	//SEEK_SET,SEEK_CUR,SEEK_END
 *			off_t l_len;	//加锁区域的长度
 *			pid_t l_pid;	//PID of process blocking our lock
 *
 *		}
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FILE_NAME	"test_file"

/*文件记录锁操作的封装*/
int lock_set(int fd, int type) {
	struct flock lock;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_type = type;
	lock.l_pid = -1;

	/* 判断是否能给文件上锁 */
	fcntl(fd, F_GETLK, &lock);
	if (lock.l_type != F_UNLCK) {
		if (lock.l_type == F_RDLCK) {
			printf("Read lock already set by %d\r\n", lock.l_pid);
		} else if (lock.l_type == F_WRLCK) {
			printf("Write lock already set by %d\r\n", lock.l_pid);
		} else {
			printf("Unknow lock type\r\n");
			return -1;
		}
	}
	/* 一定要把锁的类型重新改变回来,之前可能已经被内核修改了 */
	lock.l_type = type;
	/* 根据不同的type值进行阻塞式上锁或解锁 */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(F_SETLKW)");
		return -1;
	}
	/* 打印上锁后的信息 */
	switch (lock.l_type) {
	case F_RDLCK:
		printf("Read Lock set by %d\r\n", getpid());
		break;
	case F_WRLCK:
		printf("Write Lock set by %d\r\n", getpid());
		break;
	case F_UNLCK:
		printf("Release Lock by %d\r\n", getpid());
		break;
	default:
		break;
	}

	return 0;
}

int main() {
	int fd;
	int ret;
	/* 1. 以读写方式打开文件 */
	fd = open(FILE_NAME, O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	/* 2. 设置写入锁 */
	ret = lock_set(fd, F_WRLCK);
	if (ret < 0) {
		printf("set write lock failed\r\n");
	}
	/* 等待用户按键输入，此处用于等待测试 */
	getchar();
	/* 3. 释放文件记录锁 */
	ret = lock_set(fd, F_UNLCK);
	if (ret < 0) {
		printf("unlock failed\r\n");
	}
	/* 等待用户按键输入，此处用于等待测试 */
	getchar();
	/* 4. 关闭文件 */
	close(fd);

	return 0;
}

