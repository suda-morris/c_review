/*
 * 	Serial_Port.c
 *
 *  Created on: 2016年10月26日
 *      Author: morris
 *  要求：
 *		简单串口编程:打开串口设备，设置串口参数，读写串口数据
 *	********************************************************************
 *	1. 终端有三种工作模式，分别为规范模式，非规范模式和原始模式。
 *	2. 在规范模式下，所有的输入是基于行进行处理的。在用户输入一个行结束符（回车、EOF)
 *	之前，系统调用read函数是读不到用户输入的任何字符的。
 *	3. 除了EOF之外的行结束符与普通字符一样会被read函数读取到缓冲区中。
 *	在规范模式中，一次调用read函数最多只能读取一行数据。
 *	4. 在非规范模式下，所有的输入是即时有效的，不需要用户另外输入行结束符
 *	5. 原始模式是一种特殊的非规范模式，在原始模式下，所有的输入数据以字节为单位被处理。
 *	终端是不可回显的，而且所有特定的终端输入/输出控制处理不可用
 *	********************************************************************
 *  int tcgetattr (int fd, struct termios *termios_p);获取终端设备的配置参数
 *  	fd：文件描述符,指向终端设备
 *  	termios_p：终端设备配置参数结构体指针
 *  	return：
 *  		成功：返回0
 *  		失败：返回-1,并设置errno
 *  struct termios
 *  {
 *  	tcflag_t c_iflag;		//input mode flags
 *   	tcflag_t c_oflag;		//output mode flags
 *   	tcflag_t c_cflag;		//control mode flags
 *   	tcflag_t c_lflag;		//local mode flags/
 *   	cc_t c_line;			//line discipline
 *   	cc_t c_cc[NCCS];		//control characters
 *   	speed_t c_ispeed;		//input speed
 *   	speed_t c_ospeed;		//output speed
 * 	};
 *  void cfmakeraw (struct termios *termios_p);
 *  	将终端设置为原始模式。
 *  int cfsetispeed (struct termios *termios_p, speed_t speed);
 *  	设置数据输入波特率,成功返回0,出错返回-1,并设置errno
 *  int cfsetospeed (struct termios *termios_p, speed_t speed);
 *  	设置数据输出波特率,成功返回0,出错返回-1,并设置errno
 *  	termios_p：终端设备配置参数结构体指针
 *  	speed：波特率，比如B115200
 *	int tcflush (int fd, int queue_selector);用于清空输入/输出缓冲区
 *		fd：文件描述符,指向终端设备
 *		queue_selector：
 *			TCIFLUSH:对接收到而未被读取的数据进行清空处理
 *			TCOFLUSH:对尚未传送成功的数据进行清空处理
 *			TCIOFLUSH:包括前两种功能
 *		返回：
 *  		成功：返回0
 *  		失败：返回-1,并设置errno
 *	int tcsetattr(int fd, int optional_actions,
 const struct termios *termios_p);激活串口的配置参数
 *  	fd：文件描述符,指向终端设备
 *  	optional_actions：
 *  		TCSANOW：配置的修改立即生效
 *  		TCSADRAIN：配置的修改在所有写入fd的输出都传输完成之后生效
 *  		TCSAFLUSH：所有已接收但未读入的输入都将在修改生效之前被丢弃
 *  	termios_p：终端设备配置参数结构体指针
 *  	return：
 *  		成功：返回0
 *  		失败：返回-1,并设置errno
 *  int isatty (int fd);
 *  	测试打开的文件描述符是否连接到一个终端设备，以进一步确认串口是否正确打开
 *  	fd：文件描述符
 *  	return：
 *  		是终端设备：返回1
 *  		不是终端设备：返回0,并设置errno
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define BUFFER_SIZE			1024
#define SERIAL_PORT_STR		"/dev/ttyUSB0"

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

int main() {
	int fd;
	char buf[BUFFER_SIZE];

	/* 1. 打开串口设备 */
	fd = open_port(SERIAL_PORT_STR);
	if (fd < 0) {
		printf("open port:%s failed\r\n", SERIAL_PORT_STR);
		exit(1);
	}
	/* 2. 配置串口参数:波特率115200.8位数据位，1位停止位，无奇偶校验位 */
	if (set_com_config(fd, 115200, 8, 'N', 1) < 0) {
		printf("set_com_config failed\r\n");
		close(fd);
		exit(1);
	}
	/* 3. 写串口数据 */
	memset(buf, 0, BUFFER_SIZE);
	sprintf(buf, "%s\r\n", "uname -a");
	write(fd, buf, strlen(buf));
	/* 4. 读串口数据 */
	do {
		memset(buf, 0, BUFFER_SIZE);
		if (read(fd, buf, BUFFER_SIZE) > 0) {
			printf("Received words:%s\r\n", buf);
		}
	} while (strncmp(buf, "quit", 4));
	/* 4. 关闭串口设备文件 */
	close(fd);
	return 0;
}
