/*
 * 	Thread_Semaphore.c
 *
 *  Created on: 2016年11月6日
 *      Author: morris
 *  要求：
 *  	线程间控制之---信号量
 *  	编写“消费者-生产者”问题:
 *		有一个有限缓冲区和两个线程：生产者和消费者，它们分别不停地把产品放入缓冲区和
 *		从缓冲区中拿走产品。一个生产者在缓冲区满的时候必须等待，一个消费者在缓冲区空
 *		的时候也必须等待。另外，因为缓冲区是临界资源，所以生产者和消费者之间必须互斥
 *		执行。
 *		使用有名管道模拟有限缓冲区
 *		生产者的速度要比消费者的速度平均快两倍左右
 *	********************************************************************
 * 	1. PV原子操作主要用于进程或线程之间的同步和互斥两种典型情况。
 * 	2. 当信号量用于互斥，几个进程（或线程）往往只设置一个信号量sem。
 * 	3. 当信号量用于同步操作，会设置多个信号量，安排不同的初始值来实现它们之间的顺序执行
 * 	********************************************************************
 *	int sem_init(sem_t* sem,int pshared,unsigned int value);创建一个信号量
 *		sem:信号量指针
 *		pshared:决定信号量能否在几个进程之间共享。
 *				由于目前Linux还没有实现进程间共享信号量，所以这个值只能够取0,
 *				表示这个信号量时当前进程的局部信号量
 *		value:信号量初始化值
 *		返回：
 *			成功：返回0
 *			出错：返回-1
 *	int sem_wait(sem_t* sem);
 *		相当于P操作，该函数会阻塞
 *	int sem_trywait(sem_t* sem);
 *		相当于P操作，该函数会立即返回
 *	int sem_post(sem_t* sem);
 *		相当于V操作，它将信号量的值加1，同时发出信号来唤醒等待的进程
 *	int sem_getvalue(sem_t* sem);
 *		用于得到信号量的值
 *	int sem_destroy(sem_t* sem);
 *		用于删除信号量
 *		sem:信号量指针
 *		返回：
 *			成功：返回0
 *			出错：返回-1
 */

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MYFIFO				"/tmp/myfifo"	//有名管道的名字
#define UNIT_COUNT			(3)		//缓冲区单元数
#define UNIT_SIZE			(6)		//每个单元的大小
#define RUN_TIME			(20)	//运行时间/s
#define P_DELAY_TIME_LEVELS	(2.0)	//生产周期的最大值
#define C_DELAY_TIME_LEVELS	(4.0)	//消费周期的最大值

static int fd_fifo;	//有名管道，临界资源
static sem_t mutex, full, avail; //三个信号量，mutex用于互斥，另两个用于同步
static time_t end_time;		//结束时间

/* 生产者线程 */
void* producer(void* arg) {
	int real_write;
	int delay_time;
	/* 有时间限定 */
	while (time(NULL) < end_time) {
		delay_time = (int) (rand() * P_DELAY_TIME_LEVELS / RAND_MAX) + 1;
		sleep(delay_time);
		printf("Producer:delay=%d\r\n", delay_time);
		/* P操作信号量avail和mutex */
		sem_wait(&avail);
		sem_wait(&mutex);
		/* 向有名管道写入数据 */
		real_write = write(fd_fifo, "hello", UNIT_SIZE);
		if (real_write < 0) {
			if (errno == EAGAIN) {
				printf("The fifo has not been read yet,try later.\r\n");
			} else {
				perror("write");
			}
		} else {
			printf("Write %d bytes to FIFO\r\n", real_write);
		}
		/* V操作信号量full和mutex */
		sem_post(&full);
		sem_post(&mutex);
	}
	pthread_exit(NULL);
}

/* 消费者线程 */
void* customer(void* arg) {
	int real_read;
	int delay_time;
	char buffer[UNIT_SIZE];
	/* 有时间限定 */
	while (time(NULL) < end_time) {
		delay_time = (int) (rand() * C_DELAY_TIME_LEVELS / RAND_MAX) + 1;
		sleep(delay_time);
		printf("Customer: delay=%d\r\n", delay_time);
		/* P操作信号量full和mutex */
		sem_wait(&full);
		sem_wait(&mutex);
		/* 从有名管道读出数据 */
		memset(buffer, 0, UNIT_SIZE);
		real_read = read(fd_fifo, buffer, UNIT_SIZE);
		if (real_read < 0) {
			if (errno == EAGAIN) {
				printf("No data yet.\r\n");
			} else {
				perror("read");
			}
		} else if (real_read == 0) {
			printf("FIFO has been closed\r\n");
		} else {
			printf("read %d bytes from fifo:%s\r\n", real_read, buffer);
		}
		/* V操作信号量avail和mutex */
		sem_post(&avail);
		sem_post(&mutex);
	}
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
	pthread_t thrd_prd_id, thrd_cst_id;
	int ret;
	/* 播种随机数种子 */
	srand(time(NULL));
	/* 计算结束时间 */
	end_time = time(NULL) + RUN_TIME;
	/* 判断有名管道是否存在，不存在则创建 */
	if (access(MYFIFO, F_OK) == -1) {
		if ((mkfifo(MYFIFO, 0666) < 0)) {
			perror("mkfifo");
			exit(1);
		}
	}
	/* 以读写方式打开有名管道 */
	fd_fifo = open(MYFIFO, O_RDWR);
	if (fd_fifo == -1) {
		printf("Open fifo error\r\n");
		return fd_fifo;
	}
	/* 初始化互斥信号量为1 */
	ret = sem_init(&mutex, 0, 1);
	/* 初始化full信号量为0 */
	ret += sem_init(&full, 0, 0);
	/* 初始化avail信号量为N */
	ret += sem_init(&avail, 0, UNIT_COUNT);
	if (ret != 0) {
		printf("Semaphore initialization error\r\n");
		return ret;
	}
	/* 创建生产者线程,默认属性 */
	ret = pthread_create(&thrd_prd_id, NULL, producer, NULL);
	if (ret != 0) {
		printf("Create producer thread error\r\n");
		return ret;
	}
	/* 创建消费者线程，默认属性 */
	ret = pthread_create(&thrd_cst_id, NULL, customer, NULL);
	if (ret != 0) {
		printf("Create customer thread error\r\n");
		return ret;
	}
	/* 等待子线程退出，回收资源 */
	pthread_join(thrd_prd_id, NULL);
	pthread_join(thrd_cst_id, NULL);
	/* 关闭有名管道 */
	close(fd_fifo);
	/* 删除有名管道 */
	unlink(MYFIFO);

	return 0;
}

