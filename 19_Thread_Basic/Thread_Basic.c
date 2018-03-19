/*
 * 	Thread_Basic.c
 *
 *  Created on: 2016年11月4日
 *      Author: morris
 *  要求：
 *  	Linux下基本多线程编程
 *  	线程属性设置，线程创建，线程退出，线程等待，线程资源回收
 *	********************************************************************
 *	1. 线程与进程相比的优点：
 *		a. 不同的进程具有独立的地址空间，每个进程会建立众多数据表来维护它的代码段、
 *		堆栈段和数据段，这是一种昂贵的多任务工作方式。而运行于一个进程中的多个线程
 *		彼此之间使用相同的地址空间，共享大部分数据，启动一个线程所花费的空间远远小于启
 *		动一个进程所花费的空间，线程间彼此切换所需时间也远远小于进程间切换所需要的时间
 *		b. 线程之间有着更方便的通信机制。进程之间数据传递只能通过通信的方式进行，费时且
 *		不方便。同一进程下的线程之间共享数据空间，一个线程的数据可以直接为其它线程所用
 *	2. pthread并非Linux系统的默认库，所以编译多线程程序时需要加上编译选项-lpthread
 *	3. 进程是资源分配的最小单位，而线程是调度的最小单位
 *	4. 进程有独立的代码段，数据段和堆栈段，而线程只有独立的堆栈段
 *	5. 调用pthread_join通常是为了在线程结束后能够释放线程的资源。但是调用它后，如果该
 *	线程没有运行结束，调用者会被阻塞，可以在父线程中调用pthread_detach(thread_id)
 *	或者在子线程中调用pthread_detach(pthread_self())将子线程的状态设置为detached
 *	分离状态（默认为joinable状态），则该线程运行结束后会自动释放所有资源
 *	6. 线程的多项属性都是可更改的，这些属性主要包括绑定属性,分离属性,堆栈属性,堆栈大小
 *	及优先级。其中系统默认的属性为非绑定、非分离、默认1MB的堆栈及父进程同样级别的优先级
 * 	7. 绑定属性
 * 		Linux中采用“一对一”的线程机制，也就是一个用户线程对应一个内核线程。
 * 		绑定属性就是指一个用户线程固定地分配给一个内核线程，因为CPU时间片的调度是面向
 * 		内核线程的，因此具有绑定属性的线程可以保证在需要的时候总有一个内核线程与之对应。
 * 		而与之对应的非绑定属性就是指用户线程和内核线程的关系不是始终固定的，而是由系统
 * 		来控制分配的
 * 	8. 分离属性
 * 		分离属性用来决定一个线程以什么样的方式来终止。在非分离情况下，当一个线程结束时，
 * 		它所占用的系统资源并没有被释放，也就是没有真正终止，只有当pthread_join()函数
 * 		返回时，创建的线程才能释放自己占用的系统资源。而在分离属性情况下，一个线程结束时
 * 		立即释放它所占有的系统资源。这里需要注意，如果设置一个线程的分离属性，而这个线程
 * 		运行又非常快，那么它可能在pthread_create()函数返回之前就终止了，它终止以后就
 * 		可能将线程号和系统资源交给其他的线程使用，这时调用pthread_create()的线程就得
 * 		到了错误的线程号
 * 	9. 在不考虑因运行出错而退出的前提下，如何保证线程终止时能顺利释放掉自己所占用的资源:
 *		将待处理的线程代码放在pthread_cleanup_push()和pthread_cleanup_pop()之
 *		间，当程序运行在它们之间发生终止动作（包括调用函数pthread_exit和非正常退出）
 *		时，都将执行pthread_cleanup_push()函数所指定的清理函数进行线程清除工作
 *	10. pthread_cleanup_push()指定的清理函数调用的时机只有下述三种情况：
 *		a. 调用pthread_exit函数
 *		b. 响应取消请求时（包括其他异常退出）
 *		c. 调用参数非0的pthread_cleanup_pop函数
 *	11. 因此，在pthread_cleanup_push和pthread_cleanup_pop之间使用return返回
 *	是不会进行线程清除的
 * 	********************************************************************
 * 	int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
 void *(*start_routine) (void *), void *arg);创建一个线程
 * 		thread:线程标识符指针
 * 		attr：线程属性设置，如果没有特殊需求，通常取值为NULL
 * 		start_routine:线程运行函数的起始地址
 * 		arg：传递给start_routine的参数
 * 		返回：
 * 			成功：返回0
 * 			出错：返回错误码
 * 	void pthread_exit(void* retval);主动退出线程，但是占用的资源不会随着线程的
 * 	终止而得到释放
 * 		retval:线程将诶数时候的返回值，可由其他函数，如pthread_join来获取
 * 	int pthread_join(pthread_t thread, void **retval);将当前线程挂起来等待
 * 	线程的结束。是一个线程阻塞函数。当函数返回，被等待线程的资源也就会被回收
 * 		thread:等待线程的标识符
 * 		retval:用户定义的指针，用来存储被等待线程结束时候的返回值
 * 		返回：
 * 			成功：返回0
 * 			出错：返回错误码
 *	void pthread_cleanup_push(void (*routine)(void *),
 void *arg);压栈线程清理函数，标记一个线程清理段的开始
 *	 routine：线程清除函数
 *	 arg：传递给routine函数的参数
 * 	void pthread_cleanup_pop(int execute);标记一个线程清理段的结束
 *		execute:非0时，调用此函数会执行配对pthread_cleanup_push中指定的清理函数;
 *				为0时，调用此函数不会调用执行清理函数
 * 	int pthread_cancel(pthread_t th);在当前线程中终止另一个线程，但在被取消的
 * 	线程内部需要调用pthread_setcancel()函数和pthread_setcalceltype()函数来
 * 	设置自己的取消状态。例如，被取消的线程接收到另一个线程的取消请求之后，是接受还是
 * 	忽略这个请求;如果接受，则再判断是立即采取终止操作还是等待某个函数的调用
 * 		th:要取消的线程的标识符
 * 		返回：
 * 			成功：返回0
 * 			出错：返回出错码
 * 	int pthread_attr_init(pthread_attr_t *attr);初始化线程属性结构体
 * 		attr:线程属性结构体
 * 		返回：
 * 			成功：返回0
 * 			出错：返回错误码
 *	int pthread_attr_setscope(pthread_attr_t* attr,int scope);
 *	设置线程的绑定属性
 *		attr:线程属性结构体指针
 *		scope:
 *			PTHREAD_SCOPE_SYSTEM：绑定
 *			PTHREAD_SCOPE_PROCESS:非绑定
 *		返回：
 *			成功：返回0
 *			出错：返回错误码
 *	int pthread_attr_setdetachstate(pthread_attr_t* attr,
 int detachstate);设置线程的分离属性
 *		attr:线程属性
 *		detachstate:
 *			PTHREAD_CREATE_DETACHED:分离
 *			PTHREAD_CREATE_JOINABLE：非分离
 *		返回：
 *			成功：返回0
 *			出错：返回错误码
 *	int pthread_attr_getschedparam(pthread_attr_t* attr,
 struct sched_param* param);获取线程优先级
 *		attr:线程属性结构体指针
 *		param:线程优先级
 *		返回：
 *			成功：返回0
 *			出错：返回错误码
 *	int pthread_attr_setschedparam(pthread_attr_t* attr,
 struct sched_param* param);设置线程优先级
 *		attr:线程属性结构指针
 *		param:线程优先级
 *		返回：
 *			成功：返回0
 *			出错：返回错误码
 *	int pthread_attr_destroy(pthread_attr_t *attr);
 *	对分配的属性结构指针进行清理和回收
 *		attr:线程属性结构体
 * 		返回：
 * 			成功：返回0
 * 			出错：返回错误码
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define THREAD_NUMBER		4
#define REPEAT_NUMBER		5
#define DELAY_TIME_LEVELS	3.0

/* 线程3由于分离属性，主线程没法通过pthread_join函数来判断线程是否已经结束 */
static int thrd3_finish_flag = 0;

/* 线程清理函数 */
void* clean(void* arg) {
	printf("Thread cleanup:%s\r\n", (char*) arg);
	return (void*) 0;
}

/* 线程函数 */
void* thrd_func(void* arg) {
	int thrd_num = (int) arg;
	int delay_time = 0;
	int i = 0;
	printf("Thread %d is staring\r\n", thrd_num);
	/* 将线程清理函数压入清除栈 */
	pthread_cleanup_push((void*)clean,"push arguement");
	/* 模拟线程任务 */
	for (i = 0; i < REPEAT_NUMBER; i++) {
		delay_time = (int) (rand() * DELAY_TIME_LEVELS / (RAND_MAX)) + 1;
		sleep(delay_time);
		printf("Thread %d:job %d,delay=%d\r\n", thrd_num, i, delay_time);
	}
	printf("Thread %d finished\r\n", thrd_num);
	/* 线程3具有分离属性，主线程无法用pthread_join函数获知该线程是否已经退出 */
	if (thrd_num == 3) {
		thrd3_finish_flag = 1;
	}
	/* 退出线程，执行线程清理函数 */
	pthread_exit(arg);
	pthread_cleanup_pop(0);
}

int main(int argc, char **argv) {
	pthread_t thread[THREAD_NUMBER];
	int ret;
	void* thrd_ret;
	int i;
	pthread_attr_t attr;

	/* 播种随机数种子 */
	srand(time(NULL));
	/* 线程0、1、2使用默认属性创建 */
	for (i = 0; i < THREAD_NUMBER - 1; i++) {
		ret = pthread_create(&thread[i], NULL, thrd_func, (void*) i);
		if (ret != 0) {
			printf("create thread %d error(%d)\r\n", i, ret);
			exit(ret);
		}
	}
	/*  线程3使用自定义属性进行创建 */
	pthread_attr_init(&attr);
	/* 设置分离属性 */
	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	/* 设置绑定属性 */
	ret += pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	if (ret != 0) {
		printf("Set Thread attribute error\r\n");
		exit(ret);
	}
	/* 创建线程3 */
	ret = pthread_create(&thread[i], &attr, thrd_func, (void*) i);
	if (ret != 0) {
		printf("create thread %d error(%d)\r\n", i, ret);
		exit(ret);
	}
	/* 释放线程属性对象 */
	pthread_attr_destroy(&attr);

	printf("Create threads successfully\r\n");
	printf("Waiting for threads to finish...\r\n");
	for (i = 0; i < THREAD_NUMBER - 1; i++) {
		/* 等待线程结束，回收线程资源 */
		ret = pthread_join(thread[i], &thrd_ret);
		if (ret != 0) {
			printf("Thread %d join failed\r\n", i);
		} else {
			printf("Thread %d joined,ret=%d\r\n", i, (int) thrd_ret);
		}
	}
	/* 通过全局变量来判断分离属性的线程是否已经结束 */
	while (!thrd3_finish_flag) {
		printf("Waiting for thread %d to finish\r\n", i);
		sleep(1);
	}
	printf("Thread %d finished\r\n", i);

	return 0;
}
