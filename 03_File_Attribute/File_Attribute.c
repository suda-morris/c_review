/*
 * File_Attribute.c
 *
 *  Created on: 2016年11月8日
 *      Author: morris
 *  要求：
 *		打印任意文件的属性，包括类型、访问权限、修改时间
 *  ********************************************************************
 *  1. stat与lstat的区别在于，对于链接文件，stat返回链接文件指向的实际文件的属性，
 *  而lstat返回链接文件本身的属性
 *  ********************************************************************
 *  int stat(const char* file,struct stat* buf);获取文件的属性
 *	int lstat (const char *file,struct stat* buf);获取文件的属性
 *	int fstat(int fd,struct stat* buf);获取文件的属性
 *		fd:文件描述符
 *		file：文件名
 *		buf：文件属性结构体指针
 *		返回：
 *			成功：返回0
 *			出错：返回-1,并设置errno
 *	struct stat {
 *  	dev_t     st_dev;         //ID of device containing file
 *      ino_t     st_ino;         //inode number
 *      mode_t    st_mode;        //protection
 *      nlink_t   st_nlink;       //number of hard links
 *    	uid_t     st_uid;         //user ID of owner
 *      gid_t     st_gid;         //group ID of owner
 *      dev_t     st_rdev;        //device ID (if special file)
 *      off_t     st_size;        //total size, in bytes
 *      blksize_t st_blksize;     //blocksize for filesystem I/O
 *      blkcnt_t  st_blocks;      //number of 512B blocks allocated
 *      struct timespec st_atim;  //time of last access
 *      struct timespec st_mtim;  //time of last modification
 *      struct timespec st_ctim;  //time of last status change
 *      #define st_atime st_atim.tv_sec //Backward compatibility
 *      #define st_mtime st_mtim.tv_sec
 *      #define st_ctime st_ctim.tv_sec
 * 	};
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

void usage(char* arg) {
	printf("Usage:%s <file name>\r\n", arg);
}

int main(int argc, char **argv) {
	struct stat file_stat;
	int n;
	struct tm* tp;
	int ret;

	/* 1. 参数检查 */
	if (argc < 2) {
		usage(argv[0]);
		exit(1);
	}
	/* 2. 查看文件属性 */
	ret = lstat(argv[1], &file_stat);
	if (ret < 0) {
		perror("lstat");
		exit(1);
	}
	/* 3. 判断文件类型 */
	switch (file_stat.st_mode & S_IFMT) {
	case S_IFSOCK:
		printf("s");
		break;
	case S_IFLNK:
		printf("l");
		break;
	case S_IFREG:
		printf("-");
		break;
	case S_IFBLK:
		printf("b");
		break;
	case S_IFDIR:
		printf("d");
		break;
	case S_IFCHR:
		printf("c");
		break;
	case S_IFIFO:
		printf("p");
		break;
	default:
		printf(" ");
		break;
	}
	/* 4. 判断文件的访问权限 */
	for (n = 8; n >= 0; n--) {
		if (file_stat.st_mode & (1 << n)) {
			switch (n % 3) {
			case 2:
				printf("r");
				break;
			case 1:
				printf("w");
				break;
			case 0:
				printf("x");
				break;
			default:
				printf(" ");
				break;
			}
		} else {
			printf("-");
		}
	}
	/* 5. 打印文件大小 */
	printf(" %lu", file_stat.st_size);
	/* 6. 查看上次修改时间 */
	tp = localtime((time_t*) (&file_stat.st_mtim));
	printf(" %d-%02d-%02d %02d:%02d:%02d", tp->tm_year + 1900, tp->tm_mon + 1,
			tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
	/* 7. 打印文件名 */
	printf(" %s\r\n", argv[1]);

	return 0;
}

