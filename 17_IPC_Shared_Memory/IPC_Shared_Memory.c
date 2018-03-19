/*
 * 	IPC_Shared_Memory.c
 *
 *  Created on: 2016年11月4日
 *      Author: morris
 *  要求：
 *  	进程间通信之---共享内存+信号量
 *  	采用信号量作为同步机制完成两个进程（“生产者”和“消费者”）之间的通信
 *	********************************************************************
 *	1. 信号量是用来解决进程间的同步与互斥问题的一种进程间通信机制，包括一个称为信号量的
 *	变量和在该信号量下等待资源的进程等待队列，以及对信号量进行的两个原子操作（P/V操作）
 *  2. 信号量对应于某种资源，取一个非负的整型值。
 *  3. 信号量指的是当前可用的该资源的数量，若等于0则意味着目前没有可用的资源
 *  4. PV原子操作的具体定义如下:
 *  	P操作：如果有可用的资源（信号量值>0），则占用一个资源（给信号量值减1,进入临界
 *  		代码区）;如果没有可用的资源（信号量值=0），则被阻塞直到系统将资源分配给该
 *  		进程（进入等待队列，一直等到资源轮到该进程）
 *		V操作：如果在该信号量的等待队列中有进程在等待资源，则唤醒一个阻塞进程如果没有
 *			进程等待它，则释放一个资源（给信号量值加1）
 *	5. 共享内存是最为高效的进程间通信方式，进程可以直接读写内存，不需要任何数据的复制。
 *	6. 为了在多个进程间交换信息，内核专门留出了一块内存区，这段内存区可以由需要访问的
 *	进程将其映射到自己的私有地址空间。
 *	7. 共享内存的实现可分为两个步骤：
 *		第一步：创建共享内存，shmget()
 *		第二步：映射共享内存，shmat()，此时就可以使用不带缓冲的I/O读写命令对其进行操作
 *	8. 如果要彻底销毁在系统中创建的共享内存，需要使用shmctl(shmid,IPC_RMID,0)来
 *	标记指定的共享内存，但只有在所有与这块共享内存由映射关系的进程都脱离映射后这块共享
 *	内存才真正删除。脱离共享内存的方式是调用shmdt函数，另外，调用shmat的进程结束后也会
 *	自动脱离该共享内存
 *	9. 使用命令ipcs -m，可以查看到系统中存在的共享内存的情况，这些共享内存一旦创建除非
 *	使用shmctl(shmid,IPC_RMID,0)显式销毁，或者重启电脑，将会一直存在于系统中
 *	********************************************************************
 *	key_t ftok(const char *pathname, int proj_id);通过文件路径名和子序号，
 *		获得System V IPC键值（即创建消息队列，信号量，共享内存所用的键值）
 *		pathname:指定的带路经的文件名
 *		proj_id:子序号id或称工程id。如果指定文件的索引节点号为0x010002,而指定的ID
 *				为0x26,则最后返回的key_t值为0x26010002
 *		返回：
 *			成功：返回System V IPC键值
 *			出错：返回-1,并设置errno
 *	int semget(key_t key, int nsems, int semflg);创建信号量，
 *	或者获得在系统中已经存在的信号量
 *		key:信号量的键值，多个进程可以通过它访问同一个信号量，其中有个特殊值
 *			IPC_PRIVATE，用于创建当前进程的私有信号量
 *		nsems:需要创建的信号量数目，通常取值为1
 *		semflg:同open函数的权限位，也可以使用八进制表示法，其中
 *			使用IPC_CREATE标志创建新的信号量，即使该信号量已经存在，也不会出错。
 *			如果同时使用IPC_EXCL标志可以创建一个新的唯一的信号量
 *		返回：
 *			成功：返回信号量的标识符，在信号量的其他函数中都会使用该值
 *			出错：返回-1,并设置errno
 *	int semctl(int semid, int semnum, int cmd, union semun arg);信号量控制
 *		semid:semget()函数返回的信号量标识符
 *		semnum：信号量编号，当使用信号量集时才会被用到。
 *			通常取值为0,就是使用单个信号量（也是第一个信号量）
 *		cmd：指定对信号量的各种操作
 *			IPC_STAT：获得该信号量（或信号量集）的semid_ds结构，
 *				结果存放在由第4个参数arg结构变量的buf域指向的semid_ds结构中
 *			SETVAL：将信号量值设置为arg的val值
 *			GETVAL：返回信号量的当前值
 *			IPC_RMID：从系统中删除信号量（或者信号量集）
 *		arg：是union semun结构，在某些系统中不给出该结构的定义，必须由程序员自己定义
 *			union semun
 *			{
 *				int val;
 *				struct semid_ds *buf;
 *				unsigned short *array;
 *			}
 *		返回：
 *			成功：根据cmd值的不同而返回不同的值
 *				IPC_STAT、IPC_SETVAL、IPC_RMID返回0
 *				IPC_GETVAL返回信号量的当前值
 *			出错：返回-1，并设置errno
 *	int semop(int semid, struct sembuf* sops, size_t nsops)信号量操作
 *		semid:semget()函数返回的信号量标识符
 *		sops：指向信号量操作数组
 *		nsops：操作数组sops中的操作个数（元素数目）
 *		返回：
 *			成功：返回信号量标识符
 *			出错：返回-1,并设置errno
 *	struct sembuf
 *	{
 *		short sem_num;//信号量编号，使用单个信号量时，通常取值0
 *		short sem_op;//信号量操作：取值-1表示P操作，取值+1表示V操作
 *		short sem_flg;//通常设置为SEM_UNDO。这样在进程没释放信号量而退出时，
 *						系统自动释放该进程中未释放的信号量
 *	}
 *	int shmget(key_t key, int size, int shmflg);分配共享内存
 *		key:共享内存的键值，多个进程可以通过它访问同一个共享内存，其中有个特殊值
 *			IPC_PRIVATE，用于创建当前进程的私有共享内存
 *		size:共享内存的大小
 *		shmflg:同open函数的权限位，也可以使用八进制表示法
 *		返回：
 *			成功：返回共享内存段标识符
 *			出错：返回-1，并设置errno
 *	void* shmat(int shmid, const void* shmaddr, int shmflg);映射共享内存
 *		shmid：要映射的共享内存区标识符
 *		shmaddr：将共享内存映射到指定地址
 *			（若为0则表示系统自动分配地址并把该段共享内存映射到调用进程的地址空间）
 *		shmflg:
 *			SHM_RDONLY：共享内存只读
 *			默认0：共享内存可读可写
 *		返回：
 *			成功：返回被映射的段地址
 *			出错：返回-1，并设置errno
 *	int shmdt(const void* shmaddr);使共享内存脱离映射的进程的地址空间
 *		shmaddr:被映射的共享内存段地址
 *		返回：
 *			成功：返回0
 *			出错：返回-1，并设置errno
 *	int shmctl(int shmid, int cmd, struct shmid_ds *buf);控制共享内存
 *		shmid：共享内存ID
 *		cmd：控制命令
 *			IPC_STAT：得到共享内存的状态
 *			IPC_SET：改变共享内存的状态
 *			IPC_RMID：删除共享内存
 *		buf：是一个结构体指针，IPC_STAT的时候，取得的状态放在这个结构体中。如果要
 *			改变共享内存的状态，用这个结构体指定
 *		返回：
 *			成功：返回0
 *			出错：返回-1,并设置errno
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_BUFF_SIZE		512

/* 自定义共享内存中每个实体 */
struct shm_entity {
	int pid;
	char buffer[SHM_BUFF_SIZE];
};

/* union semun联合体需要程序员自己定义 */
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

/* 信号量初始化函数 */
int init_sem(int sem_id, int init_value) {
	union semun sem_union;
	int ret;
	/* init_value为初始值 */
	sem_union.val = init_value;
	ret = semctl(sem_id, 0, SETVAL, sem_union);
	if (ret == -1) {
		perror("semctl(SETVAL)");
		return -1;
	}
	return 0;
}

/* 从系统中删除信号量 */
int del_sem(int sem_id) {
	union semun sem_union;
	int ret;
	/* 删除信号量 */
	ret = semctl(sem_id, 0, IPC_RMID, sem_union);
	if (ret == -1) {
		perror("semctl(IPC_RMID)");
		return -1;
	}
	return 0;
}

/* P操作函数 */
int sem_p(int sem_id) {
	struct sembuf sem_buf;
	int ret;

	sem_buf.sem_num = 0;	//单个信号量的编号应该为0
	sem_buf.sem_op = -1;	//表示P操作
	sem_buf.sem_flg = SEM_UNDO;	//系统自动释放系统中残留的信号量
	ret = semop(sem_id, &sem_buf, 1);
	if (ret == -1) {
		perror("semop");
		return -1;
	}
	return 0;
}

/* V操作函数 */
int sem_v(int sem_id) {
	struct sembuf sem_buf;
	int ret;

	sem_buf.sem_num = 0;	//单个信号量的编号应该为0
	sem_buf.sem_op = 1;	//表示V操作
	sem_buf.sem_flg = SEM_UNDO;	//系统自动释放系统中残留的信号量
	ret = semop(sem_id, &sem_buf, 1);
	if (ret == -1) {
		perror("semop");
		return -1;
	}
	return 0;
}

/* 忽略一些信号，以免非法退出程序 */
int ignore_signals() {
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGSTOP, SIG_IGN);
	return 0;
}

/* 程序用法提示 */
void Usage(char* arg) {
	printf("Usage:%s w/W/r/R\r\n", arg);
}

int main(int argc, char **argv) {
	void* shared_memory = NULL;
	struct shm_entity* shm_buff_inst;
	int shmid, semid;
	/* 参数检测 */
	if (argc <= 1) {
		Usage(argv[0]);
		exit(1);
	}
	/* 防止程序非正常退出 */
	ignore_signals();
	/* 生产者程序 */
	if ((strncasecmp(argv[1], "w", 1) == 0)) {
		/* 定义信号量，用于实现访问共享内存进程之间的互斥 */
		semid = semget(ftok(".", 'a'), 1, 0666 | IPC_CREAT);
		if (semid < 0) {
			perror("semget");
			exit(1);
		}
		/* 初始化信号量的值为1 */
		init_sem(semid, 1);
		/* 创建共享内存 */
		shmid = shmget(ftok(".", 'b'), sizeof(struct shm_entity),
				0666 | IPC_CREAT);
		if (shmid < 0) {
			perror("shmget");
			/* 不要忘了把信号量给删除 */
			del_sem(semid);
			exit(1);
		}
		/* 将共享内存地址映射到当前进程地址空间,可读可写 */
		shared_memory = shmat(shmid, (void*) 0, 0);
		if (shared_memory == (void*) (-1)) {
			perror("shmat error\r\n");
			del_sem(semid);
			exit(1);
		}
		printf("Memory attached at %p\r\n", shared_memory);
		shm_buff_inst = (struct shm_entity*) shared_memory;
		/* 循环读取用户的终端输入，写入共享内存中 */
		do {
			/* P操作 */
			sem_p(semid);
			printf("Produce some text('quit' to leave):\r\n");
			if (!fgets(shm_buff_inst->buffer, SHM_BUFF_SIZE, stdin)) {
				perror("fgets");
				/* 意外退出，释放信号量 */
				sem_v(semid);
				exit(1);
			}
			shm_buff_inst->pid = getpid();
			/* V操作 */
			sem_v(semid);
		} while (strncmp(shm_buff_inst->buffer, "quit", 4));
		/* 删除信号量，信号量谁创建的就由谁负责回收 */
		del_sem(semid);
		/* 共享内存脱离进程的地址空间 */
		if (shmdt(shared_memory) < 0) {
			perror("shmdt");
			exit(1);
		}
		exit(0);
	}
	/* 消费者程序 */
	else if ((strncasecmp(argv[1], "r", 1) == 0)) {
		/* 获得信号量 */
		semid = semget(ftok(".", 'a'), 1, 0666);
		if (semid < 0) {
			perror("semget");
			exit(1);
		}
		/* 获得共享内存 */
		shmid = shmget(ftok(".", 'b'), sizeof(struct shm_entity), 0666);
		if (shmid < 0) {
			perror("shmget");
			exit(1);
		}
		/* 将共享内存地址映射到当前进程地址空间,可读可写 */
		shared_memory = shmat(shmid, (void*) 0, 0);
		if (shared_memory == (void*) (-1)) {
			perror("shmat");
			exit(1);
		}
		printf("Memory attached at %p\r\n", shared_memory);
		shm_buff_inst = (struct shm_entity*) shared_memory;
		/* 循环读取共享内存中的数据 */
		do {
			/* P操作 */
			sem_p(semid);
			printf("Shared memory was written by process %d:%s\r\n",
					shm_buff_inst->pid, shm_buff_inst->buffer);
			if (strncmp(shm_buff_inst->buffer, "quit", 4) == 0) {
				break;
			}
			shm_buff_inst->pid = 0;
			memset(shm_buff_inst->buffer, 0, SHM_BUFF_SIZE);
			/* V操作 */
			sem_v(semid);
		} while (1);
		/* 共享内存脱离进程的地址空间 */
		if (shmdt(shared_memory) < 0) {
			perror("shmdt");
			exit(1);
		}
		/* 删除共享内存 */
		if (shmctl(shmid, IPC_RMID, NULL) < 0) {
			perror("shmctl(IPC_RMID)");
			exit(1);
		}
		exit(0);
	} else {
		Usage(argv[0]);
		exit(1);
	}

	return 0;
}

