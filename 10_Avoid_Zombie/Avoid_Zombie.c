/*
 * 	Avoid_Zombie.c
 *
 *  Created on: 2016年10月31日
 *      Author: morris
 *  要求：
 *		使用信号处理机制对进程资源进行回收，避免僵尸进程的产生
 *		同时也避免wait函数阻塞主进程
 *	********************************************************************
 *	1. 一个进程在调用exit命令结束自己的生命的时候，其实它并没有真正被销毁，而是留下
 *	一个被称为僵尸进程（Zombie）的数据结构
 *	2. 在Linux进程的状态中，僵尸进程是非常特殊的一种，它已经放弃了几乎所有的内存空间，
 *	没有任何可执行代码，也不能被调度，仅仅在进程列表保留一个位置，记载该进程的退出状态等
 *	信息供其他进程收集，除此以外，僵尸进程不再占用任何内存空间
 *	3.它需要它的父进程来为它收尸，如果它的父进程没有安装SIGCHILD信号处理函数调用wait
 *	或者waitpid等等待子进程结束，又没有显式忽略该信号，那么它就会一直保持僵尸状态，
 *	如果这时父进程结束了，那么init进程会自动接手这个子进程，为它收尸，他还是能被清除的。
 *	但是如果父进程是一个循环，不会结束，那么子进程会一直保持僵尸状态。
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* 回收子进程的信号处理函数 */
void sig_child_handler(int signo) {
	wait(NULL);	//回收资源
	printf("SIGCHILD received!\r\n");
	exit(0);
}

int main() {
	pid_t pid;
	/* 1. 为SIGCHILD信号绑定信号处理函数 */
	if (signal(SIGCHLD, sig_child_handler) == SIG_ERR) {
		perror("signal(SIGCHILD)");
		exit(1);
	}
	/* 2. 创建子进程 */
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	} else if (pid == 0) {	//子进程
		sleep(3);
		exit(0);
	} else {	//父进程，每秒打印一次字符串
		while (1) {
			printf("I am father\r\n");
			sleep(1);
		}
	}
	return 0;
}
