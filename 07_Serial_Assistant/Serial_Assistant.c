/*
 * Serial_Assistant.c
 *
 *  Created on: 2016年11月14日
 *      Author: morris
 *  要求：
 *		利用select多路复用和串口编程实现一个简单的串口助手功能
 */

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#define BUFFER_SIZE			1024
#define SERIAL_PORT_STR		"/dev/ttyUSB0"
#define SEL_FILE_NUM		2
#define RECV_FILE_NAME		"recv.dat"
#define MAX(a,b)			((a>b)?(a):(b))
#define TIME_DELAY			(10)

/* 简单的串口设置封装 */
int set_com_config(int fd, int baud_rate, int data_bits, char parity,
		int stop_bits) {
	struct termios new_cfg, old_cfg;
	int speed;
	int ret;
	/* 保存并测试现有串口参数设置，在这里如果串口号等出错，会有相关的出错信息 */
	if (tcgetattr(fd, &old_cfg) < 0) {
		perror("tcgetattr");
		return -1;
	}

	new_cfg = old_cfg;
	/* 配置为原始模式 */
	cfmakeraw(&new_cfg);
	/* 设置波特率 */
	switch (baud_rate) {
	case 2400:
		speed = B2400;
		break;
	case 4800:
		speed = B4800;
		break;
	case 9600:
		speed = B9600;
		break;
	case 19200:
		speed = B19200;
		break;
	case 38400:
		speed = B38400;
		break;
	case 115200:
		speed = B115200;
		break;
	default:
		speed = B115200;
		break;
	}
	ret = cfsetispeed(&new_cfg, speed);
	ret += cfsetospeed(&new_cfg, speed);
	if (ret < 0) {
		perror("cfsetispeed or cfsetospeed");
		return -1;
	}
	/* 设置数据位 */
	new_cfg.c_cflag &= ~CSIZE;
	switch (data_bits) {
	case 7:
		new_cfg.c_cflag |= CS7;
		break;
	case 8:
		new_cfg.c_cflag |= CS8;
		break;
	default:
		new_cfg.c_cflag |= CS8;
		break;
	}
	/* 设置奇偶校验位 */
	switch (parity) {
	case 'o':
	case 'O':
		new_cfg.c_cflag |= (PARODD | PARENB);
		new_cfg.c_iflag |= INPCK;
		break;
	case 'e':
	case 'E':
		new_cfg.c_cflag |= PARENB;
		new_cfg.c_cflag &= ~PARODD;
		new_cfg.c_iflag |= INPCK;
		break;
	case 'n':
	case 'N':
		new_cfg.c_cflag &= ~PARENB;
		new_cfg.c_iflag &= ~INPCK;
		break;
	default:
		new_cfg.c_cflag &= ~PARENB;
		new_cfg.c_iflag &= ~INPCK;
		break;
	}
	/* 设置停止位 */
	switch (stop_bits) {
	case 1:
		new_cfg.c_cflag &= ~CSTOPB;
		break;
	case 2:
		new_cfg.c_cflag |= CSTOPB;
		break;
	default:
		new_cfg.c_cflag &= ~CSTOPB;
		break;
	}
	/*设置等待时间和最小接收字符*/
	new_cfg.c_cc[VTIME] = 0;
	new_cfg.c_cc[VMIN] = 1;
	/* 清除未接收数据 */
	if (tcflush(fd, TCIFLUSH) < 0) {
		perror("tcflush");
		return -1;
	}
	/* 激活串口配置 */
	if (tcsetattr(fd, TCSANOW, &new_cfg) < 0) {
		perror("tcsetattr");
		return -1;
	}

	return 0;
}

/* 打开串口功能封装 */
int open_port(char* devname) {
	int fd;
	int flags;

	/* 打开串口设备 */
	/* O_NOCTTY标志表示该参数不会使打开的文件成为进程的控制终端 */
	/* O_NDELAY标志用于设置非阻塞方式，表示不关心DCD信号线所处的状态 */
	fd = open(devname, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		perror("open");
		return -1;
	}
	/*恢复串口的状态为阻塞状态*/
	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) {
		perror("fcntl F_GETFL");
		return -1;
	}
	flags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) {
		perror("fcntl F_SETFL");
		return -1;
	}
	/* 测试打开的文件是否为终端设备 */
	if (isatty(fd) == 0) {
		perror("This is nor a terminal device");
		return -1;
	}

	return fd;
}

int main(int argc, char **argv) {
	char buf[BUFFER_SIZE];
	int real_read;
	int fds[SEL_FILE_NUM];
	fd_set inset, tmp_inset;
	struct timeval tv;
	int max_fd;
	int ret;
	int i;

	/* 标准输入STDIN_FILENO（0） */
	fds[0] = STDIN_FILENO;
	/* 1. 打开串口设备 */
	fds[1] = open_port(SERIAL_PORT_STR);
	if (fds[1] < 0) {
		printf("open port:%s failed\r\n", SERIAL_PORT_STR);
		exit(1);
	}
	/* 2. 配置串口参数*/
	ret = set_com_config(fds[1], 115200, 8, 'N', 1);
	if (ret < 0) {
		printf("set_com_config failed\r\n");
		close(fds[1]);
		exit(1);
	}
	/* 3. 设置输入描述符集合inset */
	FD_ZERO(&inset);
	FD_SET(fds[0], &inset);
	FD_SET(fds[1], &inset);
	/* 求最大文件描述符 */
	max_fd = MAX(fds[0], fds[1]);
	/* 5. 循环读取来自终端或者串口设备的数据 */
	printf("Input('quit' to exit):\r\n");
	while (FD_ISSET(fds[0],&inset) || FD_ISSET(fds[0], &inset)) {
		/* 设置超时时间 */
		tv.tv_sec = TIME_DELAY;
		tv.tv_usec = 0;
		tmp_inset = inset;
		ret = select(max_fd + 1, &tmp_inset, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select");
			exit(1);
		} else if (ret == 0) {
			printf("Time-out\r\n");
			exit(1);
		} else {
			for (i = 0; i < SEL_FILE_NUM; i++) {
				if (FD_ISSET(fds[i], &tmp_inset)) {
					memset(buf, 0, BUFFER_SIZE);
					/* 读标准输入或者串口设备文件 */
					real_read = read(fds[i], buf, BUFFER_SIZE);
					if ((real_read < 0) && (errno != EAGAIN)) {
						perror("read");
						exit(1);
					} else if (real_read == 0) {
						close(fds[i]);
						FD_CLR(fds[i], &inset);
					} else {
						buf[real_read] = '\0';
						/* 将终端读到的数据写入串口 */
						if (i == 0) {
							write(fds[1], buf, strlen(buf));
							printf("Input('quit' to exit):\r\n");
						} else {
							/* 将串口收到的数据发送到终端 */
							printf("Receive:%s\r\n", buf);
						}
						if (strncmp(buf, "quit", 4) == 0) {
							exit(0);
						}
					}
				}
			}
		}
	}

	return 0;
}

