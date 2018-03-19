/*
 * 	IPC_Signal.c
 *
 *  Created on: 2016年11月2日
 *      Author: morris
 *  要求：
 *  	进程间通信之---信号Signal
 *  	使用signal捕捉信号,使用alarm函数模拟sleep操作
 *	********************************************************************
 *  1. 信号是软件层面上对中断机制的一种模拟。在原理上，一个进程收到一个信号与处理器收到
 *  一个中断请求可以说是一样的。
 *  2. 信号是异步的，一个进程不必通过任何操作来等待信号的到达
 *  3. 信号可以直接进行用户空间进程和内核进程之间的交互，内核进程也可以利用它来通知用户
 *  空间进程发生了哪些系统事件。
 *  4. 信号是进程间通信机制中唯一的异步通信机制，可以看做是异步通知，通知接收信号的进程
 *  有哪些事情发生了。
 *  5. 信号事件的发生有两个来源：
 *  	a. 硬件来源（如键盘上的按钮或者出现其他硬件故障）
 *  	b. 软件来源，最常用发送信号的系统函数有kill、raise、alarm、setitimer
 *  	和sigqueue等，软件来源还包括一些非法运算操作等
 *  6. 进程可以通过3种方式来响应一个信号
 *  	a. 忽略信号，即对信号不作任何处理，其中有两个信号不能忽略：SIGKILL和SIGSTOP
 *  	b. 捕捉信号，定义信号处理函数，当信号发生时，执行相应的处理函数
 *  	c. 执行默认操作，Linux对每种信号都定义了默认操作
 *  		SIGHUP：该信号在用户终端连接（正常或非正常）结束时发出，通常是在终端的
 *  		控制进程结束时，通知同一会话内的各个进程与控制终端不再关联
 *  			默认操作：终止
 *  		SIGINT：该信号在用于输入INTR字符（通常是Ctrl+C）时发出，终端驱动程序
 *  		发送此信号并送到前台进程中的每一个进程
 *  			默认操作：终止
 *  		SIGQUIT：该信号和SIGINT类似，但由QUIT字符（通常是Ctrl+\）来控制
 *  			默认操作：终止
 *  		SIGABRT:调用abort函数生成的信号
 *  		SIGILL：该信号在一个进程企图执行一条非法指令时（可执行文件本身出现错误，
 *  		或者试图执行数据断、堆栈溢出时）发出
 *  			默认操作：终止
 *  		SIGFPE：该信号在发生致命的算数运算错误时发出。这里不仅包括浮点运算错误，
 *  		还包括溢出及除数为0等其他所有的算数错误
 *  			默认操作：终止
 *  		SIGKILL：该信号是用来立即结束程序的运行，并且不能被阻塞、处理和忽略
 *  			默认操作：终止
 *  		SIGALRM：该信号用于通知进程定时器时间已到
 *  			默认操作：终止
 *  		SIGSTOP：该信号用于暂停一个进程，且不能被阻塞、处理或忽略
 *  			默认操作：暂停进程
 *  		SIGTSTP：该信号用于交互停止进程，用户在输入SUSP字符时（通常是Ctrl+Z）
 *  		发出这个信号给终端的前台进程，让前台进程在后台挂起，该信号可以被处理和忽略
 *  			默认操作：停止进程
 *  		SIGCONT:让一个停止的进程继续执行，本心好不能被阻塞
 *  			默认操作：继续运行
 *  		SIGCHILD：子进程改变状态的时候，父进程会收到这个信号
 *  			默认操作：忽略
 *  		SIGSEGV: 该信号在非法访问内存时产生，如野指针、缓冲区溢出
 *  			默认操作：终止
 *  		SIGPIPE：当进程往一个没有读端的管道中写入时产生，代表“管道断裂”
 *  			默认操作：终止
 *  		SIGTERM:程序结束信号，与SIGKILL不同的是该信号可以被阻塞和处理，通常用来
 *  		要求程序自己正常退出，shell命令kill缺省产生这个信号。如果进程终止不了，我
 *  		们才会尝试SIGKILL
 *  		SIGUSR1/2：该进程保留给用户程序使用
 *  			默认操作：终止
 *  7. 一个进程只能有一个闹钟时间，如果在调用alarm之前已经设置过闹钟时间，则任何以前
 *  的闹钟时间都被新值所代替
 *  8. 信号处理的方法有两种，一种是使用signal函数，另一种是使用信号集函数组
 *  9. 可以使用kill [-s signal | -signal] pid来向进程号或进程组号为pid的进程
 *  发送signal信号(可以使信号宏名或者对应编号,通过kill -l可以查看到)
 *  ********************************************************************
 * 	int kill(pit_t pid, int sig);发送信号给进程或进程组
 *  	pid:
 *  		正数：要发送信号的进程号
 *  		0：信号被发送到所有和当前进程在同一个进程组的进程
 *  		-1：信号发送给所有进程表中的进程（除了init进程和当前进程）
 *  		<-1:信号发送给进程组号为-pid的每一个进程
 *  	sig:信号
 *  	返回：
 *  		成功：返回0
 *  		出错：返回-1，并设置errno
 *  int raise(int sig);进程向自己发送信号
 *  	sig:信号
 *  	返回：
 *  		成功：返回0
 *  		出错：返回-1
 *  unsigned int alarm(unsigned int seconds);闹钟函数，它可以在进程中设置
 *  			一个定时器，当定时器指定的时间到时，它就向进程发送SIGALARM信号。
 *		seconds:指定秒数，系统经过seconds秒后向该进程发送SIGALARM信号。
 *				如果seconds为0,表示取消当前定时器
 *		返回：
 *			成功：如果调用此alarm前进程中已经设置了闹钟时间，则返回上一个闹钟时间的
 *				剩余时间，否则返回0
 *			出错：返回-1
 *  int pause(void);将调用进程挂起直至捕捉到信号为止，这个函数很常用，
 *  				通常可以用于判断信号是否已经到达
 *  	返回：只返回-1,并设置errno为EINTR（4）
 *  sighandler_t signal(int signum,sighandler_t handler);
 *  	signum:指定信号代码
 *  	handler：
 *  		SIG_IGN：忽略该信号
 *  		SIG_DFL:采用系统默认方式处理信号
 *  		自定义的信号处理函数指针
 *  	返回：
 *  		成功：返回以前的信号处理函数
 *  		出错：返回SIG_ERR(-1),并设置errno
 *  typedef void (*sighandler_t)(int);
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/* 自定义信号处理函数 */
void my_sig_handler(int signo) {
	if (signo == SIGALRM) {
		printf("I have get SIGALARM\r\n");
	} else if (signo == SIGINT) {
		printf("I have get SIGINT\r\n");
	} else if (signo == SIGQUIT) {
		printf("I have get SIGQUIT\r\n");
	}
}

int main() {
	pid_t pid;
	int ret;
	/* 创建一个子进程 */
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	} else if (pid == 0) {
		printf("Child(pid:%d) starts\r\n", getpid());
		/* 在子进程中使用raise函数发出SIGSTOP信号，使子进程暂停 */
		raise(SIGSTOP);
		exit(0);
	} else {
		/* 以下代码完成了一个简单的sleep函数的功能 */
		/* 注册SIGALARM信号的处理函数 */
		signal(SIGALRM, my_sig_handler);
		/* 定时2秒 */
		alarm(2);
		/* 等待信号到来,不出意外最先到来的信号应该是SIGALARM */
		pause();
		/* 定时2秒结束，进程继续往下走 */
		/* 在父进程中收集子进程发出的信号，并调用kill执行相应的操作 */
		if (waitpid(pid, NULL, WNOHANG) == 0) {	//子进程还没退出
			printf("Waiting for child to exit\r\n");
			/* 主动杀死子进程 */
			ret = kill(pid, SIGKILL);
			if (ret == 0) {
				printf("Parent kill %d\r\n", pid);
			} else {
				perror("kill");
				exit(1);
			}
		}
		/* 发送完SIGKILL信号后，阻塞等待子进程退出 */
		waitpid(pid, NULL, 0);
		printf("Child(%d) exited\r\n", pid);

		printf("Waiting for signal SIGINT or SIGQUIT...\r\n");
		/* 注册信号处理函数 */
		signal(SIGINT, my_sig_handler);
		signal(SIGQUIT, my_sig_handler);
		/* 阻塞等待信号到来 */
		pause();
		exit(0);
	}
	return 0;
}

