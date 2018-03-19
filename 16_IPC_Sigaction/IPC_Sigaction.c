/*
 * 	IPC_Sigaction.c
 *
 *  Created on: 2016年11月2日
 *      Author: morris
 *  要求：
 *  	进程间通信之---信号Sigaction
 *  	信号处理实例
 *	********************************************************************
 *	1. 在处理信号时，一般遵循的操作流程如下：
 *		定义信号集--->设置信号屏蔽位--->定义信号处理函数--->测试信号
 *	2. sigaction函数比signal函数更加健壮，推荐使用该函数
 *	3. signal是标准C的信号接口，而sigaction是POSIX信号接口
 *	4. sigaction的信号处理函数可以采用void (*sa_handler)(int)或
 *	void (*sa_sigaction)(int, siginfo_t *, void *)。到底采用哪种要看sa_flags
 *	中是否设置了SA_SIGINFO位，如果设置了就采用后者，此时可以向处理函数发送附加信息
 *	5. 注册信号处理函数主要用于决定进程如何处理信号。需要注意，信号集里的信号并不是真正
 *	可以处理的信号，只有当信号的状态处于非阻塞状态时才会真正起作用
 *	6. 检测信号是信号处理的后续步骤，因为被阻塞的信号不会传递给进程，所以这些信号就处于
 *	“未处理”状态，sigpending函数允许进程检测“未处理”信号，并进一步决定对它们做何处理
 *	********************************************************************
 *	int sigaction(int signum, const struct sigaction *act,
 struct sigaction *oldact);
 *  	signum:信号代码，可以为除SIGKILL及SIGSTOP之外的任何一个特定有效的信号
 *  	act：指向结构sigaction的一个实例的指针，指定对特定信号的处理
 *  	oldact：保存原来对应信号的处理
 *  	返回：
 *  		成功：返回0
 *  		出错：返回-1，并设置errno
 *  struct sigaction
 *  {
 *  	void (*sa_handler)(int signo);//指定信号处理函数
 *  	void (*sa_sigaction)(int, siginfo_t *, void *);//指定信号处理函数
 *  	sigset_t sa_mask;//指定在信号处理程序执行过程中哪些信号应当被屏蔽
 *  	int sa_flags;	//包含了对信号进行处理的各个选项。
 *  					//SA_NODEFER，当捕捉到此信号时，
 *  	在执行其信号捕捉函数时，系统不会自动屏蔽此信号
 *  					//SA_NOCLDSTOP，进程忽略子进程产生的任何SIGSTOP、
 *  	SIGTSTP、SIGTTIN和SIGTTOU信号
 *  					//SA_RESTART，如果信号中断了进程的某个系统调用，
 *  	则系统自动启动该系统调用
 *  					//SA_ONESHOT/SA_RESETHAND，自定义信号只执行一次，
 *  	在执行完毕后恢复信号的系统默认动作
 *  	void (*sa_restore)(void);//此参数没有使用
 *  }
 *	int sigemptyset(sigset_t* set);将信号集初始化为空
 *		set:信号集
 *		返回：
 *			成功：返回0
 *			出错：返回-1，并设置errno
 *	int sigfillset(sigset_t* set);将信号集初始化为包含所有已经定义的信号集
 *		set:信号集
 *		返回：
 *			成功：返回0
 *			出错：返回-1，并设置errno
 *	int sigaddset(sigset_t* set,int signum);将指定信号加入到信号集中
 *		set:信号集
 *		signum:指定信号代码
 *		返回：
 *			成功：返回0
 *			出错：返回-1，并设置errno
 *	sigdelset(sigset_t* set,int signum);将指定信号从信号集中删除
 *		set:信号集
 *		signum:指定信号代码
 *		返回：
 *			成功：返回0
 *			出错：返回-1，并设置errno
 *	sigismember(sigset_t* set,int signum);查询指定信号是否在信号集中
 *		set:信号集
 *		signum:指定信号代码
 *		返回：
 *			成功：返回1，表示已经存在于信号集中
 *			失败：返回0,表示不再信号集中
 *			出错：返回-1
 *	int sigprocmask(int how,const sigset_t* set,sigset_t* oset);
 *	检测并更改信号屏蔽字（信号屏蔽字是用来指定当前被阻塞的一组信号，它们不会被进程接收）
 *		how:
 *			SIG_BLOCK：增加一个信号集到当前进程的阻塞集中
 *			SIG_UNBLOCK：从当前的阻塞集中删除一个信号集
 *			SIG_SETMASK：将当前的信号集设置为信号阻塞集
 *		set:指定信号集
 *		oset:信号屏蔽字
 *		返回：
 *			成功:返回0
 *			出错：返回-1，并设置errno
 *	int sigpending(sigset_t* set);
 *		set:要检测的信号集
 *		返回：
 *			成功：返回0
 *			出错：返回-1
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* 自定义信号处理函数 */
void my_sig_handler(int signo) {
	printf("If you want to quit, please try SIGQUIT(Ctrl+\\)\r\n");
}

int main(int argc, char **argv) {
	sigset_t set;
	struct sigaction action1, action2;
	int ret;
	/* 1. 初始化信号集为空 */
	ret = sigemptyset(&set);
	if (ret < 0) {
		perror("sigemptyset");
		exit(1);
	}
	/* 2. 将相应的信号加入信号集 */
	ret = sigaddset(&set, SIGINT);
	if (ret < 0) {
		perror("sigaddset(SIGINT)");
		exit(1);
	}
	ret = sigaddset(&set, SIGQUIT);
	if (ret < 0) {
		perror("sigaddset(SIGQUIT)");
		exit(1);
	}
	/* 3. 指定信号处理方式 */
	/* SIGINT信号被自定义处理函数捕捉 */
	if (sigismember(&set, SIGINT)) {
		sigemptyset(&action1.sa_mask);
		action1.sa_handler = my_sig_handler;
		action1.sa_flags = 0;
		sigaction(SIGINT, &action1, NULL);
	}
	/* SIGQUIT信号默认处理方式，退出 */
	if (sigismember(&set, SIGQUIT)) {
		sigemptyset(&action2.sa_mask);
		action2.sa_handler = SIG_DFL;
		action2.sa_flags = 0;
		sigaction(SIGQUIT, &action2, NULL);
	}
	/* 4. 设置信号集屏蔽字，此时set中的信号不会被传递给进程，暂时进入等待处理状态 */
	ret = sigprocmask(SIG_BLOCK, &set, NULL);
	if (ret < 0) {
		perror("sigprocmask(SIG_BLOCK)");
		exit(1);
	} else {
		printf("Signal set was blocked, Press any key to release\r\n");
		getchar();
	}
	/* 5. 在信号屏蔽字中删除set中的信号 */
	ret = sigprocmask(SIG_UNBLOCK, &set, NULL);
	if (ret < 0) {
		perror("sigprocmask(SIG_UNBLOCK)");
		exit(1);
	}
	printf("Signal set is in unblock state\r\n");
	while (1)
		;
	return 0;
}

