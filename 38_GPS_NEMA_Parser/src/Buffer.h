/*
 ============================================================================
 Name        : Buffer.h
 Author      : morris
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BUFFER_FLAG_INITIALIZED     (1 << 0)
#define BUFFER_FLAG_MALLOC          (1 << 1)

#ifndef LIB_ALLOC_FUNC
#define LIB_ALLOC_FUNC         		malloc
#endif
#ifndef LIB_FREE_FUNC
#define LIB_FREE_FUNC          		free
#endif

/* 缓冲区结构体 */
typedef struct _BUFFER_t {
	uint32_t Size; //缓冲区数据Buffer指针指向区域的大小，单位：字节
	uint32_t In; //输入指针
	uint32_t Out; //输入指针
	uint8_t* Buffer; //缓冲区数据指针
	uint8_t Flags; //缓冲区的标志
	uint8_t StringDelimiter; //字符串分隔符
	void* UserParameters; //用户数据指针，可选
} BUFFER_t;

#define BUFFER_SetStringDelimiter(Buffer, StrDel)  ((Buffer)->StringDelimiter = (StrDel))

/**
 * 初始化Buffer数据结构，如果BufferPtr为空，会在堆上另开Size大小的内存
 * @param  Buffer    Buffer_t对象指针
 * @param  Size      缓冲区数据大小
 * @param  BufferPtr 缓冲区数据区域指针
 * @return           成功返回0，Buffer对象为空返回-1，分配内存出错返回-2
 */
int8_t BUFFER_Init(BUFFER_t* Buffer, uint32_t Size, void* BufferPtr);

/**
 * 释放在BUFFER_Init中开辟的内存
 * @param Buffer Buffer_t对象指针
 */
void BUFFER_Free(BUFFER_t* Buffer);

/**
 * 复位Buffer的读写指针
 * @param Buffer Buffer_t对象指针
 */
void BUFFER_Reset(BUFFER_t* Buffer);

/**
 * 获取Buffer中空闲内存大小
 * @param  Buffer Buffer_t对象指针
 * @return        空闲内存大小
 */
uint32_t BUFFER_GetFree(BUFFER_t* Buffer);

/**
 * 获取Buffer中已经利用的内存大小
 * @param  Buffer Buffet-t对象指针
 * @return        已经利用的内存大小
 */
uint32_t BUFFER_GetFull(BUFFER_t* Buffer);

/**
 * 向Buffer中写入数据
 * @param  Buffer Buffer_t对象指针
 * @param  Data   写入数据的指针
 * @param  count  要写入数据的大小
 * @return        返回成功写入数据的大小
 */
uint32_t BUFFER_Write(BUFFER_t* Buffer, const void* Data, uint32_t count);

/**
 * 向Buffer中写入字符串
 * @param  Buffer Buffer_t对象指针
 * @param  buff   写入的字符串
 * @return        返回成功写入的数据的大小
 */
uint32_t BUFFER_WriteString(BUFFER_t* Buffer, const char* buff);

/**
 * 写入缓冲区的头部
 * @param  Buffer Buffer_t对象指针
 * @param  Data   数据指针
 * @param  count  写入数据的大小
 * @return        成功写入数据的大小
 */
uint32_t BUFFER_WriteToTop(BUFFER_t* Buffer, const void* Data, uint32_t count);

/**
 * 从Buffer中读取数据
 * @param  Buffer Buffer_t对象指针
 * @param  Data   读取到的数据
 * @param  count  想要读取的数据大小
 * @return        实际读取到的数据大小
 */
uint32_t BUFFER_Read(BUFFER_t* Buffer, void* Data, uint32_t count);

/**
 * 在Buffer中寻找元素
 * @param  Buffer  Buffet_t对象指针
 * @param  Element 待寻找的元素
 * @return         成功返回该元素相对读取指针的偏移，失败返回-1
 */
int32_t BUFFER_FindElement(BUFFER_t* Buffer, uint8_t Element);

/**
 * 返回指定偏移位置的元素
 * @param  Buffer  Buffer_t对象指针
 * @param  pos     相对读取指针的偏移
 * @param  element 读取到的数据
 * @return         成功返回0，失败返回-1
 */
int8_t BUFFER_CheckElement(BUFFER_t* Buffer, uint32_t pos, uint8_t* element);

/**
 * 从Buffer中读取字符串
 * @param  Buffer   Buffer_t对象指针
 * @param  buff     读取到的字符串
 * @param  buffsize 要读取的字符串长度
 * @return          实际读取的字符串长度
 */
uint32_t BUFFER_ReadString(BUFFER_t* Buffer, char* buff, uint32_t buffsize);

/**
 * 从Buffer中判断是否存在指定的字符序列
 * @param  Buffer Buffer_t对象指针
 * @param  Data   指定的数据序列
 * @param  Size   指定的数据序列长度
 * @return        成功返回该序列在Buffer中相对读取指针的偏移，失败返回-1
 */
int32_t BUFFER_Find(BUFFER_t* Buffer, const void* Data, uint32_t Size);

#ifdef __cplusplus
}
#endif

#endif /* BUFFER_H_ */
