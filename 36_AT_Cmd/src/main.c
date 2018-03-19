/*
 * main.c
 *
 *  Created on: 2015年12月10日
 *      Author: morris
 */
#include <stdio.h>
#include <stdlib.h>
#include "ATcmd.h"

static void sys_restart() {
	at_debug("Now will restart...\r\n");
}

static void at_queryCmdWho(tATNode* pNode) {
	at_response("I'm morris\r\n");
	at_backOk();
}

int main() {
	printf("Atcmd library\r\n");
	at_init();
	at_regCallback(sys_restart);
	at_cmdProcess("\r\n");
	at_cmdProcess("+GMR\r\n");
	at_cmdProcess("E0\r\n");
	at_cmdProcess("+RST\r\n");

	at_funcationType *newCmd = (at_funcationType*) malloc(
			sizeof(at_funcationType));
	newCmd->at_cmdName = "+WHO";
	newCmd->at_cmdLen = 4;
	newCmd->at_exeCmd = NULL;
	newCmd->at_testCmd = NULL;
	newCmd->at_setupCmd = NULL;
	newCmd->at_queryCmd = at_queryCmdWho;
	at_regCommand(newCmd);
	at_cmdProcess("+WHO?\r\n");
	at_destory();
	return 0;
}

