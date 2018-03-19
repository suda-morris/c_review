/*
 * 	Multi_Task.c
 *
 *  Created on: 2016年10月27日
 *      Author: morris
 *  要求：
 *		简单多进程编程:
 *		父进程创建两个子进程，其中一个子进程执行env命令，另一个子进程暂停5秒后退出
 *		父进程先用阻塞的方式等待第一个子进程结束，然后用非阻塞的方式等待第二个子进程退出
 *	********************************************************************
 * 	1. fork()函数用于创建一个子进程，该子进程几乎复制了父进程的全部内容，但是一个新
 * 	创建的进程如何执行？exec函数族提供了一个在进程中启动另一个程序的执行的方法。
 * 	2. exec函数族根据指定的文件名或目录名找到可执行文件，并用它来取代原调用进程的数据段
 * 	代码段和堆栈段，在执行完之后，原调用进程的内容除了进程号之外，其他全部被新的进程替换
 * 	3. 在Linux中使用exec函数族主要有两种情况：
 * 		a. 当进程认为自己不能再为系统和用户做出任何贡献的时候，就可以调用exec函数族
 * 		中的任意一个函数让自己重生
 * 		b. 如果一个进程想执行另一个程序，那么它就可以调用fork函数新建一个进程，然后调用
 * 		exec函数族中的任意一个函数，这样看起来就像通过执行应用程序而产生了一个新进程
 * 	4. 事实上，exec函数族，6个函数中真正的系统调用只有execve()，其余都是库函数
 * 	5. exit函数与_exit函数最大的区别在于exit函数在终止当前进程之前要查看进程打开过
 * 	哪些文件，并把文件缓冲区中的内容写回文件。
 * 	********************************************************************
 *  pid_t fork(void);创建新进程，并完整地复制了整个地址空间，因此执行速度比较慢
 *  	返回：
 *  		子进程：返回0
 *  		父进程：返回子进程ID（大于0）
 *  		出错：返回-1,并设置errno
 *  pid_t vfork(void);创建新进程，但不会产生父进程的副本，
 *  当子进程需要改变内存中的数据时才复制父进程，即“写操作时复制（copy on write）”
 *  	返回：
 *  		子进程：返回0
 *  		父进程：返回子进程ID（大于0）
 *  		出错：返回-1,并设置errno
 *	exec函数族成员，其中l：list，v：vector，p：path，e：environment
 *	int execl(const char* path, const char* arg,...);
 *		按照完整的文件目录路径path查找，逐个列举的方式传递参数
 *	int execv(const char* path, char* const argv[]);
 *		按照完整的文件目录路径path查找，指针数组的方式传递参数
 *	int execle(const char* path,const char* arg,...,char* const envp[]);
 *		按照完整的文件目录路径path查找，逐个列举的方式传递参数
 *	int execve(const char* path, char* const argv[], char* const envp[]);
 *		按照完整的文件目录路径path查找，指针数组的方式传递参数
 *	int execlp(const char* file, const char* arg, ...);
 *		按照系统环境变量PATH制定的路径查找文件file，逐个列举的方式传递参数
 *	int execvp(const char* file, char* const argv[]);
 *		按照系统环境变量PATH制定的路径查找文件file，指针数组的方式传递参数
 *			path：文件路径
 *			file：文件名称
 *			arg，argv：传递的参数，即使用该可执行文件的时候所需要的全部命令选项字符
 *			串，包括命令本身。要注意，这些参数必须以NULL结束
 *			envp：指定的环境变量数组,以NULL结尾
 *			返回(只有出错的时候才返回)：
 *				出错：返回-1,并设置errno
 *	void exit(int status)
 *	void _exit(int status)
 *		status:进程结束时的状态，一般来说0表示正常结束，其他数值表示出现了错误
 *	pid_t wait(int* status)用于使父进程阻塞，直到一个子进程结束或者该进程接收到一个
 *	指定的信号为止。如果该父进程没有子进程或者子进程已经结束，则wait立即返回
 *		status:
 *			子进程退出时候的状态
 *		返回：
 *			成功：返回已经结束运行的子进程的进程号
 *			失败：返回-1，并设置errno
 *	pid_t waitpid(pid_t pid, int* status,int options)等待特定的子进程结束
 *		pid:
 *			pid>0：只等待进程ID等于pid的子进程
 *			pid=-1：等待任何一个子进程退出，此时作用和wait一样
 *			pid=0：等待其组ID等于调用进程的组ID的任意一个子进程
 *			pid<0:等待其组ID等于pid的绝对值的任何一个子进程
 *		status:
 *			子进程退出时候的状态
 *		options:
 *			WNOHANG:若由pid指定的子进程没有结束，则waitpid不阻塞而立即返回，
 *			此时返回值为0
 *			WUNTRACED：为了实现某种操作，由pid指定的任意一个子进程已经被暂停，
 *			且其状态自暂停以来还未报告过，则返回其状态
 *			0：同wait，阻塞父进程，等待子进程退出
 *		返回：
 *			正常：返回已经结束运行的子进程的进程号
 *			使用选项WNOHANG且没有子进程退出：返回0
 *			调用出错：返回-1
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
	pid_t child1, child2, child;
	char* envp[] = { "PATH=/tmp", "USER=morris", NULL };

	/* 1. 创建子进程child1 */
	child1 = fork();
	if (child1 == -1) {
		perror("fork");
		exit(1);
	} else if (child1 == 0) {	//子进程child1执行env命令
		printf("I am the child1 process, My PID=%d\r\n", getpid());
		if (execle("/usr/bin/env", "env", NULL, envp) < 0) {
			perror("execle");
			exit(1);
		}
	} else {	//父进程
		/* 2. 创建子进程child2 */
		child2 = fork();
		if (child2 < 0) {
			perror("fork");
			exit(1);
		} else if (child2 == 0) {	//子进程child2暂停5s
			printf("I am the child2 process, My PID=%d\r\n", getpid());
			sleep(5);
			exit(0);
		} else {	//父进程
			printf("I am the father process, My PID=%d\r\n", getpid());
			/* 3. 以阻塞方式等待子进程1退出 */
			child = waitpid(child1, NULL, 0);
			if (child == child1) {
				printf("Get child1 exit code\r\n");
			} else {
				printf("Some error occured\r\n");
				exit(1);
			}
			/* 4. 以非阻塞方式等待子进程2退出 */
			do {
				child = waitpid(child2, NULL, WNOHANG);
				if (child == 0) {
					printf("Father:The Child2 process has not exited\r\n");
					sleep(1);
				}
			} while (child == 0);

			if (child == child2) {
				printf("Get child2 exit code\r\n");
			} else {
				printf("Some error occured\r\n");
				exit(1);
			}
		}
	}

	return 0;
}
