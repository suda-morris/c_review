/*
 * 	Thread_Mutex.c
 *
 *  Created on: 2016年11月5日
 *      Author: morris
 *  要求：
 *  	线程间控制之---互斥锁
 *  	实现多个线程有序执行
 *	********************************************************************
 *  1. 由于线程共享进程的资源和地址空间，因此在对这些资源进行操作的时候，必须考虑到线程
 *  间资源访问的同步与互斥问题。Posix中有两种线程同步机制：互斥锁和信号量
 *  2. 互斥锁适用于同时可用的资源是唯一的情况，信号量适用于同时可用的资源为多个的情况
 *  3. 互斥锁可以分为：
 *  	a. 快速互斥锁，调用线程会阻塞直到拥有互斥锁的线程解锁为止
 *  	b. 递归互斥锁能够成功返回，并且增加调用线程在互斥上加锁的次数
 *  	c. 检错互斥锁为快速互斥锁的非阻塞版本，它会立即返回并返回一个错误信息
 *  ********************************************************************
 *  int pthread_mutex_init(pthread_mutex_t* mutex,
 const pthread_mutexattr_t* mutexattr);互斥锁初始化
 *  	mutex:互斥锁
 *  	mutexattr:
 *  		PTHREAD_MUTEX_INITIALIZER：创建快速互斥锁
 *  		PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP：创建递归互斥锁
 *  		PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP：创建检错互斥锁
 *  	返回：
 *  		成功：返回0
 *  		出错：返回出错码
 *  int pthread_mutex_lock(pthread_mutex_t* mutex);互斥锁上锁
 *  int pthread_mutex_trylock(pthread_mutex_t* mutex);互斥锁判断上锁
 *  int pthread_mutex_unlock(pthread_mutex_t* mutex);互斥锁解锁
 *  int pthread_mutex_destroy(pthread_mutex_t* mutex);消除互斥锁
 *  	mutex:互斥锁
 *  	返回：
 *  		成功：返回0
 *  		出错：返回-1
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define THREAD_NUMBER		3
#define REPEAT_NUMBER		5
#define DELAY_TIME_LEVELS	3.0	//小任务之间的最大时间间隔

/* 互斥锁，因为各个线程都需要访问，所以设置为全局变量 */
pthread_mutex_t mutex;

/* 线程执行函数 */
void* thrd_func(void* arg) {
	int thrd_num = (int) arg;
	int delay_time = 0;
	int i = 0;
	int ret;
	/* 互斥锁上锁 */
	ret = pthread_mutex_lock(&mutex);
	if (ret < 0) {
		printf("Thread %d lock failed\r\n", thrd_num);
		pthread_exit(arg);
	}
	printf("Thread %d is staring\r\n", thrd_num);
	for (i = 0; i < REPEAT_NUMBER; i++) {
		delay_time = (int) (rand() * DELAY_TIME_LEVELS / (RAND_MAX)) + 1;
		sleep(delay_time);
		printf("Thread %d:job %d delay=%d\r\n", thrd_num, i, delay_time);
	}
	printf("Thread %d finished\r\n", thrd_num);
	/* 互斥锁解锁 */
	pthread_mutex_unlock(&mutex);
	/* 退出线程 */
	pthread_exit(arg);
}

int main(int argc, char **argv) {
	pthread_t thread[THREAD_NUMBER];
	int ret;
	void* thrd_ret;
	int i;
	/* 播种随机数种子 */
	srand(time(NULL));
	/* 互斥锁初始化 */
	pthread_mutex_init(&mutex, NULL);
	/* 创建线程0、1、2,采用默认的线程属性 */
	for (i = 0; i < THREAD_NUMBER; i++) {
		ret = pthread_create(&thread[i], NULL, thrd_func, (void*) i);
		if (ret != 0) {
			printf("create thread %d error\r\n", i);
			exit(ret);
		}
	}
	printf("Create threads successfully\r\n");
	printf("Waiting for threads to finish...\r\n");
	/* 等待线程退出，回事线程资源 */
	for (i = 0; i < THREAD_NUMBER; i++) {
		ret = pthread_join(thread[i], &thrd_ret);
		if (ret != 0) {
			printf("Thread %d join failed\r\n", i);
		} else {
			printf("Thread %d joined,ret=%d\r\n", i, (int) thrd_ret);
		}
	}
	/* 删除互斥锁 */
	pthread_mutex_destroy(&mutex);

	return 0;
}

