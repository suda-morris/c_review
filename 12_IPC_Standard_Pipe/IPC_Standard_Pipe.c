/*
 * 	IPC_Standard_Pipe.c
 *
 *  Created on: 2016年11月1日
 *      Author: morris
 *  要求：
 *		进程间通信之---标准流管道
 *		调用ps -ef命令，并获取命令执行后输出结果
 *	********************************************************************
 *  1. 与Linux的文件操作中有基于文件流的标准I/O操作一样，管道的操作也支持基于文件流
 *  的模式。这种基于文件流的管道主要用来创建一个连接到另一个进程的管道。
 *  2. 标准流管道将一系列的创建过程合并到一个函数popen()中完成
 *  	a. 创建一个管道
 *  	b. 使用fork函数创建一个子进程
 *  	c. 在父子进程中关闭不需要的文件描述符
 *  	d. 执行exec函数族调用
 *  	e. 执行函数中所指定的命令
 *  3. 使用popen创建的管道必须使用标准I/O函数进行操作，不能使用read，write等函数
 *  ********************************************************************
 *  FILE* popen(const char* command, const char* type)创建标准流管道
 *  	command:指向的是一个以null结束符结尾的字符串，这个字符串包含一个shell命令，
 *  			并被送到/bin/sh以-c参数执行
 *  	type:
 *  		“r”：文件指针连接到command的标准输出，即该命令的结果产生输出
 *  		“w”：文件指针连接到command的标准输入，即该命令的结果产生输入
 *  	返回：
 *  		成功：返回流指针
 *  		出错：返回NULL，并设置errno
 *  int pclose(FILE* stream)关闭标准流管道
 *  	stream：要关闭的文件流
 *  	返回：
 *  		成功：返回由popen所执行的进程的退出码
 *  		出错：返回-1,并设置errno
 */

#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE		512
#define COMMAND_STR		"ps -ef"

int main() {
	FILE* fp;
	char buf[BUFFER_SIZE];
	/* 1. 调用popen函数执行命令：ps -ef,该命令会产生输出*/
	fp = popen(COMMAND_STR, "r");
	if (fp == NULL) {
		perror("popen");
		exit(1);
	}
	/* 2. 获取命令的输出结果 */
	while (fgets(buf, BUFFER_SIZE, fp)) {
		printf("%s", buf);
	}
	/* 3. 关闭标准流管道 */
	pclose(fp);
	return 0;
}

