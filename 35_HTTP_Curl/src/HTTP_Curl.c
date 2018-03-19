/*
 * HTTP_Curl.c
 *
 *  Created on: 2016年12月9日
 *      Author: morris
 *  要求：利用libcurl实现简单的HTTP访问,将获取到的HTML页面保存到本地
 *  ********************************************************************
 *  1. 在基于libcurl的程序里，主要采用回调函数的形式完成传输任务
 *  2. libcurl的工作模式有两种，一种称为简单接口，这种模式下可进行同步、直接、快速的文件传输；
 *  李毅中称为多线程接口，多线程接口可生成多个连接线程以异步方式进行文件传输
 *  ********************************************************************
 *	CURLcode curl_global_init(long flags);对libcurl进行初始化
 *		flags:指定初始化状态
 *			CURL_GLOBAL_ALL:初始化所有可能的调用
 *			CURL_GLOBAL_SSL:初始化支持安全套接字层的调用
 *			CURL_GLOBAL_WIN32:初始化WIN32套接字库
 *			CURL_GLOBAL_NOTHING:没有额外的初始化要求
 *		返回CURL状态码
 *	void curl_global_cleanup(void);清理curl内存
 *	CURL *curl_easy_init(void);获得一个CURL操作符，通过它可以访问相应的网络资源
 *	void curl_easy_cleanup(CURL *curl);清除curl标识符
 *	CURLcode curl_easy_setopt(CURL* handle,CURLoption option,parameter);
 *		指定libcurl的工作方式
 *		handle：CURL标识符
 *		option：CURLoption类型的选项
 *		parameter：可以是回调函数的指针，也可以是某个对象的指针，或者是long类型的变量，
 *				这取决于第二个参数的定义
 *		返回CURL状态码
 *	CURLcode curl_easy_perform(CURL *curl);
 *		根据curl_easy_setopt函数指定的工作方式开始工作
 *		成功返回0.否则返回一个错误码
 */

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/typecheck-gcc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

FILE* fp;

static void printUsage(char* arg) {
	printf("Usage:%s URL_Address Local_File_Path", arg);
}

static size_t write_data(const char *ptr, size_t size, size_t nmemb,
		void* stream) {
	return fwrite(ptr, size, nmemb, fp);
}

int main(int argc, char **argv) {
	/* 参数检查 */
	if (argc < 3) {
		printUsage(argv[0]);
		exit(1);
	}
	/* 1. 定义CURL标识符指针 */
	CURL* curl;
	/* 2. 初始化CURL标识符 */
	curl_global_init(CURL_GLOBAL_ALL);
	/* 3. 获得CURL标识符 */
	curl = curl_easy_init();
	/* 4. 设置要访问的URL地址 */
	curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
	/* 5. 创建要保存的文件 */
	fp = fopen(argv[2], "w");
	if (fp == NULL) {
		printf("error Local_File_Path\r\n");
		printUsage(argv[0]);
		exit(1);
	}
	/* 6. 设置将网络文件写入本地的回调函数 */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	/* 7. 开始执行数据传输，结束后执行回调 */
	curl_easy_perform(curl);
	/* 8. 清除curl标识符 */
	curl_easy_cleanup(curl);
	/* 9. 清理curl内存 */
	curl_global_cleanup();
	return 0;
}

