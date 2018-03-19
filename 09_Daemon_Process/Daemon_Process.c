/*
 * 	Daemon_Process.c
 *
 *  Created on: 2016年10月31日
 *      Author: morris
 *  要求：
 *		简单守护进程
 *	********************************************************************
 *  a. 如何创建守护进程？
 *  	1. 创建子进程，父进程退出。由于父进程已经先于子进程退出,会造成子进程没有父进程，
 *  	从而变成一个孤儿进程。在Linux系统中，每当系统发现一个孤儿进程，就会自动由1号
 *  	进程（即init进程）收养它，这样，原先的子进程就会变成init进程的子进程
 *		2. 在子进程中创建新会话。
 *			进程组是一个或者多个进程的集合。进程组由进程组ID来唯一标识。
 *			每个进程组都有一个组长进程，其组长进程的进程号等于进程组ID，且该进程ID不会
 *			因为组长进程的退出而受到影响
 *			会话组是一个或多个进程组的集合，通常一个会话开始于用户登录，终止于用户退出，
 *			在此期间该用户运行的所有进程都属于这个会话组
 *		3. 改变当前目录为根目录
 *		4. 重设文件权限掩码为0
 *		5. 关闭父进程打开的文件描述符
 *	b. 调用setid有以下几个作用：
 *		1. 让进程摆脱原会话的控制
 *		2. 让进程摆脱原进程组的控制
 *		3. 让进程摆脱原控制终端的控制
 *	c. 因为守护进程一直在后台运行，其工作目录不能被卸载，所以
 *	要更改当前工作目录：chdir("/tmp")
 *	d. 守护进程有可能创建新文件，不希望对其创建的文件的权限加以限制，因此
 *	需要重设文件权限掩码:umask(0)
 *	e. syslog是Linux中的系统日志管理服务，通过守护进程syslogd来维护，
 *	该守护进程在启动的时候会读一个配置文件“/etc/syslog.conf”，
 *	该文件决定了不同种类的消息会发送到何处。
 *	例如紧急消息可被送到系统管理员并在控制台上显示，而警告消息则可被记录到一个文件中。
 *	f. 系统日志文件所在路径：/var/log/syslog或者/var/log/message
 *	********************************************************************
 *	pid_t setsid(void);创建一个新的会话组，并担任该会话组的组长
 *		返回：
 *			成功：返回新的会话组的ID
 *			出错：返回-1，并设置errno
 *	int chdir(const char* path);改变当前进程的工作目录
 *		path:工作路径
 *		返回：
 *			成功：返回0
 *			出错：返回-1,并设置errno
 *	__mode_t umask(__mode_t mask);设置当前进程的文件权限掩码
 *		mask：新的掩码
 *		返回：旧的掩码
 *	int getdtablesize(void);返回当前进程拥有的文件描述符的最大数量
 *	void openlog(char* ident, int option, int facility)打开系统日志服务
 *		ident:
 *			要向每个消息加入的字符串，通常为程序的名称
 *		option：
 *			LOG_CONS：如果消息无法送到系统日志服务，则直接输出到系统控制终端
 *			LOG_NDELAY：立即打开系统日志服务的链接(否则直接发送到第一条消息时才打开）
 *			LOG_PERROR：将消息同时送到stderr上
 *			LOG_PID:在每条消息中包含进程的PID
 *		facility：指定程序发送的消息类型
 *			LOG_AUTHPRIV:安全/授权信息
 *			LOG_CRON:时间守护进程
 *			LOG_DAEMON:其他系统守护进程
 *			LOG_KERN:内核信息
 *			LOG_LOCAL[0~7]:保留
 *			LOG_LPR:行打印机子系统
 *			LOG_MAIL:邮件子系统
 *			LOG_NEWS:新闻子系统
 *			LOG_SYSLOG:syslogd内部产生的信息
 *			LOG_USER:一般使用者等级信息
 *			LOG_UUCP:UUCP子系统
 *	void syslog(int priority, char* format,...)向日志文件中写入消息
 *		priority:指定消息的重要性
 *			LOG_EMERG：系统无法使用
 *			LOG_ALERT：需要立即采取措施
 *			LOG_CRIT：有重要情况发生
 *			LOG_ERR：有错误发生
 *			LOG_WARNING：有警告发生
 *			LOG_NOTICE：正常情况，但也是重要情况
 *			LOG_INFO：信息消息
 *			LOG_DEBUG：调试信息
 *		format:以字符串指针的形式表示输出格式
 *	void closelog(void)用于关闭系统日志服务的链接
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

int main() {
	pid_t pid, sid;
	int fd;
	int i;
	char* buf = "This is a Daemon\r\n";
	/* 1. 创建子进程 */
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	} else if (pid > 0) {
		/* 2. 父进程退出 */
		exit(0);
	}
	/* 一下程序仅在子进程中得到执行 */
	/* 3. 打开系统日志服务链接 */
	openlog("Daemon_Process", LOG_PID, LOG_DAEMON);
	/* 4. 创建新的会话组，子进程成为新会话组组长 */
	sid = setsid();
	if (sid < 0) {
		syslog(LOG_ERR, "%s\r\n", "setsid");
		exit(1);
	}
	/* 5. 更改当前工作目录 */
	sid = chdir("/tmp");
	if (sid < 0) {
		syslog(LOG_ERR, "%s\r\n", "chdir");
		exit(1);
	}
	/* 6. 重设文件权限掩码 */
	umask(0);
	/* 7. 关闭父进程打开的所有文件描述符，包括标准输入输出错误 */
	for (i = 0; i < getdtablesize(); i++) {
		close(i);
	}
	/* 守护进程创建完成，正式开始进入守护进程工作 */
	syslog(LOG_INFO, "create deamon success\r\n");
	while (1) {
		fd = open("/tmp/daemon.log", O_CREAT | O_WRONLY | O_APPEND, 0666);
		if (fd < 0) {
			syslog(LOG_ERR, "open /tmp/daemon.log error\r\n");
			exit(1);
		}
		write(fd, buf, strlen(buf) + 1);
		close(fd);
		sleep(10);
	}
	/* 8. 关闭系统日志 */
	closelog();

	return 0;
}

