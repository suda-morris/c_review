/*
 * ATcmd.h
 *
 *  Created on: 2015年12月10日
 *      Author: morris
 */

#ifndef ATCMD_H_
#define ATCMD_H_

#include <stdint.h>
#include <stdio.h>

//ToDo
#define AT_DEBUG
#ifdef AT_DEBUG
#define at_debug(fmt,args...)		printf(fmt,##args)
#else
#define at_debug(fmt,args...)
#endif

//ToDo
#define at_response(fmt,args...)	printf(fmt,##args)

#define AT_VERSION_main   	0x00
#define AT_VERSION_sub    	0x01
#define AT_VERSION   		(AT_VERSION_main << 8 | AT_VERSION_sub)

extern uint8_t echoFlag;	//whether to open echo
extern void (*system_restart)(void);

struct ATNode;
typedef struct {
	char *at_cmdName;
	int8_t at_cmdLen;
	void (*at_testCmd)(struct ATNode*);
	void (*at_queryCmd)(struct ATNode*);
	void (*at_setupCmd)(struct ATNode*, const char *pPara);
	void (*at_exeCmd)(struct ATNode*);
} at_funcationType;

typedef struct LinkListNode tLinkListNode;
typedef struct ATNode {
	tLinkListNode *pNext;
	at_funcationType *at_fun;
} tATNode;

#define at_backOk()        	at_response("\r\nOK\r\n")
#define at_backError()     	at_response("\r\nERROR\r\n")

void at_init(void);
void at_regCommand(at_funcationType *new_atcmd);
void at_cmdProcess(const char *pAtRcvData);
void at_regCallback(void (*sys_restart)(void));
void at_destory(void);

#endif /* ATCMD_H_ */
