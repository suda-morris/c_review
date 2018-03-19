/*
 * 	IPC_Message_Queue.c
 *
 *  Created on: 2016年11月3日
 *      Author: morris
 *  要求：
 *  	进程间通信之---消息队列
 *  	使用消息队列模拟生产者与消费者
 *	********************************************************************
 *  1. 消息队列就是一些消息的列表，用户可以在消息队列中添加消息和读取消息等。
 *  2. 消息队列具有一定的FIFO特性，但是它可以实现消息的随机查询，比FIFO具有更大的优势。
 *  同时这些消息是存在于内核中的，由“队列ID”来标识
 *  3. 消息队列与有名管道比较起来，消息队列的优点在独立于发送与接收进程，这件少了打开与
 *  关闭有名管道之间同步的困难
 *  ********************************************************************
 *  int msgget(key_t key, int msgflg);创建与打开消息队列
 *  	key:消息队列的键值，多个进程可以通过它访问同一个消息队列，其中有个特殊值
 *  	IPC_PRIVATE，用于创建当前进程的私有消息队列
 *  	msgflg：权限标志位
 *  	返回：
 *  		成功：返回消息队列ID
 *  		出错：返回-1，并设置errno
 *  int msgsnd(int msqid, const void* msgp, size_t msgsz, int msgflg);
 *  	向消息队列中发送消息
 *  	msqid:消息队列的队列ID
 *  	msgp：指向消息结构的指针，该消息结构msgbuf通常如下
 *  		struct msgbuf
 *  		{
 *  			long mtype;	//消息正文，该结构必须从这个域开始
 *  			char mtext[1];	//消息正文
 *  		}
 *  	msgsz:消息正文的字节数（不包括消息类型指针变量）
 *  	msgflg：指定处理方式
 *  		IPC_NOWAIT：若消息无法立即发送（如当前消息队列已满），函数会立即返回
 *  		0：msgsnd调用阻塞直到发送成功
 *  	返回：
 *  		成功：返回0
 *  		出错：返回-1，并设置errno
 *   ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp,
 int msgflg);从消息队列获取消息
 *  	msqid:消息队列的队列ID
 *  	msgp:消息缓冲区，同msgsnd函数的msgp
 *  	msgsz:消息正文的字节数
 *  	msgtyp:指定的接收的消息类型
 *  		0：接收消息队列中第一个消息
 *  		大于0：接收消息队列中第一个类型为msgtpy的消息
 *  		小于0：接收消息队列中第一个类型值不小于msgtyp绝对值且类型值最小的消息
 *  	msgflg:指定处理方式
 *  		MSG_NOERROR：若返回的消息比msgsz字节多，则消息就会截短到msgsz字节，
 *  					且不通知消息发送进程
 *  		IPC_NOWAIT:若在消息队列中并没有相应类型的消息可以接收，则函数立即返回
 *  		0：msgrcv()调用阻塞直到接收一条相应类型的消息为止
 *  	返回：
 *  		成功：返回0
 *  		出错：返回-1，并设置errno
 *  int msgctl(int msqid, int cmd, struct msgid_ds* buf);消息队列控制函数
 *		msqid:消息队列的队列ID
 *		cmd：命令参数
 *			IPC_STAT：读取消息队列的数据结构msqid_ds，并将其存储在buf指定的地址中
 *			IPC_SET：设置消息队列的数据结构msqid_ds中的ipc_perm域值(IPC操作权限
 *					描述结构)，这个值取自buf参数
 *			IPC_RMID：从系统内核中删除消息队列
 *		buf：描述消息队列的msqid_ds结构类型变量
 *		返回：
 *			成功：返回0
 *			出错：返回-1,并设置errno
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MSG_SIZE				512

/* 消息结构体 */
struct message {
	long msg_type; //第一个成员必须是消息类型
	char msg_text[MSG_SIZE];
};

/* 程序使用帮助 */
void Usage(char* arg) {
	printf("Usage:%s r/R/w/W\r\n", arg);
}

int main(int argc, char **argv) {
	int qid;
	key_t key;
	struct message msg;
	int ret;
	/* 参数检查 */
	if (argc <= 1) {
		Usage(argv[0]);
		exit(1);
	}
	/* 写消息队列 */
	if ((strncasecmp(argv[1], "w", 1) == 0)) {
		/* 根据不同的路径和关键字产生便准的key */
		key = ftok(".", 'a');
		if (key == -1) {
			perror("ftok");
			exit(1);
		}
		/* 创建消息队列 */
		qid = msgget(key, 0666 | IPC_CREAT);
		if (qid == -1) {
			perror("msgget");
			exit(1);
		}
		printf("Open Message Queue OK:%d\r\n", qid);
		/* 循环从用户终端读取数据写入消息队列 */
		while (1) {
			printf("Enter message to queue('quit' to leave):\r\n");
			if (fgets(msg.msg_text, MSG_SIZE, stdin) == NULL) {
				perror("fgets");
				exit(1);
			}
			msg.msg_type = getpid();
			/* 添加消息到消息队列,阻塞式 */
			ret = msgsnd(qid, &msg, strlen(msg.msg_text), 0);
			if (ret < 0) {
				perror("msgsnd");
				exit(1);
			}
			/* 判断是否要退出 */
			if (strncmp(msg.msg_text, "quit", 4) == 0) {
				break;
			}
		}
	}
	/* 读消息队列 */
	else if ((strncasecmp(argv[1], "r", 1) == 0)) {
		/* 根据不同的路径和关键字产生便准的key,要与写端一致 */
		key = ftok(".", 'a');
		if (key == -1) {
			perror("ftok");
			exit(1);
		}
		/* 创建消息队列 */
		qid = msgget(key, 0666 | IPC_CREAT);
		if (qid == -1) {
			perror("msgget");
			exit(1);
		}
		printf("Open Message Queue OK:%d\r\n", qid);
		/* 循环从消息队列中读取数据 */
		do {
			memset(msg.msg_text, 0, MSG_SIZE);
			/* 接收歇息队列中的第一个消息，阻塞式 */
			if (msgrcv(qid, &msg, MSG_SIZE, 0, 0) < 0) {
				perror("msgrcv");
				exit(1);
			}
			printf("The message from process %ld:%s\r\n", msg.msg_type,
					msg.msg_text);
		} while (strncmp(msg.msg_text, "quit", 4));
		/* 从系统中把消息队列删除 */
		ret = msgctl(qid, IPC_RMID, NULL);
		if (ret < 0) {
			perror("msgctl(IPC_RMID)");
			exit(1);
		}
	} else {
		Usage(argv[0]);
		exit(1);
	}

	return 0;
}

