/*
 * 	File_IO.c（文件IO操作）
 *  Created on: 2016年10月22日
 * 	Author: morris
 * 	要求：
 * 		将src_file文件中的最后256B内容复制到dest_file中，读写缓存大小设置为64B
 * 	********************************************************************
 *  1. 函数open(),read(),write(),lseek(),close()均为文件IO的系统调用，其特点是
 *  不带缓存，直接对文件（包括设备）进行读写操作。
 *  2. 这些函数不是ANSI C的组成部分，却是POSIX的组成部分。
 *  ********************************************************************
 *  int open(const char *pathname, int flags, mode_t mode);打开文件
 *  	pathname:被打开的文件名（可包括路径）
 *  	flags：文件打开的方式
 *  		O_RDONLY：以只读方式打开文件
 *  		O_WRONLY：以只写方式打开文件
 *  		O_RDWR：以读/写方式打开文件
 *  		O_CREATE：如果文件不存在，就创建一个新的文件，并用第三个参数为其设置权限
 *  		O_EXCL：如果使用O_CREATE时文件存在，则可返回错误消息。这一参数可以测试
 *  		文件是否存在。此时open是原子操作，防止多个进程同时创建同一个文件
 *  		O_NOCTTY：若文件为终端，那么该终端不会成为调用open()的进程的控制终端
 *  		O_TRUNC：若文件已经存在，则删除文件中的原有数据，并且设置文件大小为0
 *  		O_APPEND：以添加方式打开文件，在打开文件的同时，文件指针指向文件的末尾
 *  	mode：被打开文件的存取权限
 *  		1. 可以使用一组宏定义：S_I(R/W/X)(USR/GRP/OTH)
 *  		其中R/W/X分别表示读/写/执行权限
 *  		USR/GRP/OTH分别表示文件所有者/文件所属组/其他用户
 *  		例如S_IRUSR|S_IWUSR表示设置文件所有者的可读可写属性
 *  		2. 使用8进制数字替代，如0666表示文件对所有用户可读可写
 *  	返回：
 *  		成功：返回文件描述符（非负）
 *  		失败：返回-1,并设置errno
 *  ********************************************************************
 * 	int close(int fd);关闭文件
 *  	fd：文件描述符
 *  	返回：
 *  		成功：返回0
 *  		出错：返回-1，并设置errno
 *  ********************************************************************
 *  ssize_t read(int fd, void* buf, size_t count);读文件
 *  	fd：文件描述符
 *  	buf：指定存储器读出数据的缓冲区
 *  	count：指定读出的字节数
 *  	返回：
 *  		成功：返回读到的字节数,若到达文件末尾则返回0
 *  		出错：返回-1,并设置errno
 *  ********************************************************************
 *  ssize_t write(int fd, void* buf, size_t count)：写文件
 *  	fd：文件描述符
 *  	buf：指定存储器写入数据的缓冲区
 *  	count：指定读出的字节数
 *  	返回：
 *  		成功：返回已写入的字节数
 *  		出错：返回-1,并设置errno
 *  ********************************************************************
 *  off_t lseek(int fd, off_t offset, int whence)：文件（读写指针）定位
 *  注：只能用在可定位（可随机访问）文件操作中。管道、套接字和大部分字符设备是不可定位的
 *  	fd：文件描述符
 *  	offset：偏移量，每一读写操作所需要移动的距离，单位是字节，可正可负
 *  	whence：当前位置的基点
 *  		SEEK_SET：文件的开头
 *  		SEEK_CUR：文件指针当前的位置
 *  		SEEK_END：文件的结尾
 *  	返回：
 *  		成功：文件读写指针的当前位移(偏离文件开始多少字节)
 *  		出错：-1,并设置errno
 *  ********************************************************************
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 	(64)
#define SRC_FILE_NAME 	"src_file"
#define DEST_FILE_NAME	"dest_file"
#define OFFSET			(256)

int main() {
	int src_fd, dest_fd;
	char buff[BUFFER_SIZE];
	int real_read_len, real_write_len;
	int ret;

	/* 1. 打开文件 */
	src_fd = open(SRC_FILE_NAME, O_RDONLY);
	dest_fd = open(DEST_FILE_NAME, O_WRONLY | O_CREAT, 0666);
	if (src_fd < 0 || dest_fd < 0) {
		perror("open");
		exit(1);
	}
	/* 2. 文件定位 */
	ret = lseek(src_fd, -1 * OFFSET, SEEK_END);
	if (ret < 0) {
		perror("lseek");
		close(src_fd);
		close(dest_fd);
		exit(1);
	}
	/* 3. 读写文件*/
	while ((real_read_len = read(src_fd, buff, BUFFER_SIZE)) > 0) {
		real_write_len = write(dest_fd, buff, real_read_len);
		if (real_write_len < real_read_len) {
			perror("write");
			close(src_fd);
			close(dest_fd);
			exit(1);
		}
	}
	if (real_read_len < 0) {
		perror("read");
		close(src_fd);
		close(dest_fd);
		exit(1);
	}
	/*4. 关闭文件 */
	close(src_fd);
	close(dest_fd);

	return 0;
}

