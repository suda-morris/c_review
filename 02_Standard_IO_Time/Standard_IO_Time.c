/*
 * Standard_IO_Time.c
 *
 *  Created on: 2016年10月26日
 *      Author: morris
 *  要求：
 * 		读取当前文件夹下的内容。并每隔1秒钟将当前时间保存到文件中,并统计行数。
 * 	********************************************************************
 *  1. 标准IO操作都是基于流缓冲的，符合ANSI C的标准I/O处理。
 *  2. 标准IO提供流缓冲的目的是减少系统调用的数量，从而提高程序的效率。
 *  3. 有3种类型的缓冲存储
 *  	a. 全缓冲。当填满标准IO缓存后才进行实际IO操作。
 *  	对于存放在磁盘上的文件通常是标准IO库实施全缓冲的。
 *  	当缓冲区已经满或者手动刷新时才会进行磁盘操作。
 *  	b. 行缓冲。当在输入和输出中遇到行结束符时，标准IO库执行IO操作。
 *  	标准输入和标准输出就是使用行缓冲的典型例子。
 *  	c. 不缓冲。相当于系统调用函数。
 *  	标准出错通常是不带缓存的，出错信息能够尽快显示出来。
 *  4. Linux抽象了目录流的概念，用户可以像打开文件流一样打开目录流。
 *  	目录文件中是以目录项为单位来记录目录中包含的文件信息的。
 *  5. 日历时间，通过time_t数据类型来表示，从1970年1月1日0时0分0秒到此时的秒数
 *  ********************************************************************
 * 	FILE *fopen (const char* path,const char* mode)；
 * 		打开文件。成功返回文件流指针，失败返回NULL，并设置errno
 * 	size_t fwrite(const void *ptr, size_t size, size_t nmemb,
 FILE *stream)；
 *		按指定对象写。成功返回写入的对象的个数，失败返回-1
 * 	size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
 * 		按指定对象读。成功返回读取的对象的个数，失败返回-1
 * 	int fclose (FILE* stream)；
 * 		关闭文件流。成功返回0,失败返回-1，并设置errno
 * 	int fgetc(FILE* stream)；
 * 		字符输入。成功返回下一个字符，失败返回-1
 * 	int fputc(int c, FILE* stream)；
 * 		字符输出。成功返回字符c，失败返回-1
 * 	char* fgets(char* s, int size, FILE* stream)；
 * 		行输入。成功返回字符串s，失败返回NULL
 * 	int fputs(const char* s, FILE* stream)；
 * 		行输出。成功返回非负整数，失败返回-1
 * 	int fprintf(FILE* fp, const char* format, ...)；
 * 		格式化输出到文件流。成功返回输出的字符数，失败返回一个负数
 * 	int fscanf(FILE* fp, const char* format, ...)；
 * 		格式化从文件流中输入。成功返回输入的字符数，失败返回-1,并设置errno
 * 	int sprintf(char* buf, const char* format, ...)；
 * 		格式化输出到字符串。成功返回输出的字符数，失败返回一个负数
 * 	int sscanf(char* buf, const char* format, ...)；
 * 		格式化从字符串中输入。成功返回输入的字符数，失败返回-1,并设置errno
 * 	int fflush(FILE* fp);
 * 		强制刷新流的缓冲区（将流缓冲区中的数据写入实际的文件）
 * 		注意，在Linux下只能刷新输出缓冲区
 * 		成功返回0,失败返回-1,并设置errno
 * 	long ftell(FILE* stream);
 * 		返回当前文件流读写指针的偏移
 * 		成功时返回流的当前读写位置，出错时返回-1,并设置errno
 * 	long fseek(FILE* stream,long offset,int whence);
 * 		定位一个流，参数同lseek。成功时返回0,出错时返回-1
 * 	void rewind(FILE* stream);
 * 		把流定位到起始为止。
 * 	int ferror(FILE* stream);
 * 		返回1表示流出错，否则返回0
 * 	int feof(FILE* stream);
 * 		返回1表示文件流的读写指针已经到末尾，否则返回0
 * 	********************************************************************
 *	DIR* opendir(const char* path)；
 *		打开目录。成功返回目录流指针，失败返回NULL，并设置errno
 *	int closedir(DIR* dp)；
 *		关闭目录。成功返回0,失败返回-1,并设置errno
 *	struct dirent* readdir(DIR* dirp)；
 *		读目录。成功返回指向struct dirent结构体的指针，失败返回NULL,并设置errno
 *	struct dirent
 *	{
 *		ino_t	d_ino;//inode号
 *		off_t	d_off;//目录项偏移
 *		unsigned short d_reclen;//当前目录项大小
 *		unsigned d_type;//文件类型
 *		char	d_name;//文件名字
 *	}
 *	********************************************************************
 *	time_t time(time_t *tloc);获取当前的系统时间
 *		tloc:
 *			tloc=NULL时得到系统日历时间
 *			tloc=时间数值时，用于设置日历时间
 *		返回：
 *			成功返回日历时间，出错返回-1,并设置errno
 *	struct tm *localtime(const time_t *timep);通过日历时间获得本地时间
 *		timep:指向日历时间的指针变量
 *		返回：
 *			成功返回指向tm结构体的指针，出错返回NULL
 *	struct tm {
 *		int tm_sec;     //Seconds (0-60)
 *		int tm_min;    //Minutes (0-59)
 *		int tm_hour;   //Hours (0-23)
 *		int tm_mday;   //Day of the month (1-31)
 *		int tm_mon;    //Month (0-11)
 *		int tm_year;   //Year - 1900
 *		int tm_wday;   //Day of the week (0-6, Sunday = 0)
 *		int tm_yday;   //Day in the year (0-365, 1 Jan = 0)
 *		int tm_isdst;  //Daylight saving time
 *	};
 *	char *asctime(const struct tm *tblock);转换时期和时间为ASCII码
 *		tblock：记录时间的tm结构
 *		返回：
 *			成功返回字符串格式：星期，月，日，小时：分：秒，年
 *			出错返回NULL
 *	char *ctime(const time_t *timep);把日期和时间转换为字符串
 *		timep：指向日历时间的指针变量
 *		返回：
 *			成功返回字符串格式：星期，月，日，小时：分：秒，年
 *			出错返回NULL
 */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE		64
#define TIME_FILE		"time.msr"

int main() {
	char buf[BUFFER_SIZE];
	int lines = 0;
	struct tm* local_time;
	FILE* fp;
	time_t t;
	DIR* dp;
	struct dirent* entry;

	/* 1. 打开当前目录 */
	dp = opendir(".");
	if (dp == NULL) {
		perror("opendir");
		exit(1);
	}
	/* 打印行头 */
	printf("%20s%10s%10s%10s\r\n", "name", "inode", "len", "offset");
	/* 2. 读取目录内容 */
	do {
		entry = readdir(dp);
		if (entry) {
			printf("%20s%10ld%10d%10ld\r\n", entry->d_name, entry->d_ino,
					entry->d_reclen, entry->d_off);
		} else {
			perror("readdir");
		}
	} while (entry);
	/* 3. 关闭目录流 */
	closedir(dp);
	/* 4. 以追加读写方式创建新文件 */
	fp = fopen(TIME_FILE, "a+");
	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}
	/* 5. 计算文件中已有的行数 */
	memset(buf, 0, BUFFER_SIZE);
	while (fgets(buf, sizeof(buf), fp)) {
		if (buf[strlen(buf) - 1] == '\n') {
			lines++;
		}
		memset(buf, 0, sizeof(buf));
	}
	/* 6. 将时间保存到文件中 */
	while (1) {
		t = time(NULL);
		local_time = localtime(&t);
		fprintf(fp, "%02d->%04d年%02d月%2d日,%02d:%2dm:%02ds\r\n", ++lines,
				local_time->tm_year + 1900, local_time->tm_mon + 1,
				local_time->tm_mday, local_time->tm_hour, local_time->tm_min,
				local_time->tm_sec);
		fflush(fp);
		sleep(1);
	}
	/* 7.关闭文件流 */
	fclose(fp);
	return 0;
}

