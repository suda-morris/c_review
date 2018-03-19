/*
 * AT_BaseCmd.c
 *
 *  Created on: 2015年12月11日
 *      Author: morris
 */
#include <stdio.h>
#include "AT_BaseCmd.h"
#include "ATcmd.h"

/**
 * @brief  Execution commad of AT.
 * @param  id: commad id number
 * @retval None
 */
void at_exeCmdNull(tATNode* pNode) {
	at_debug("at_exeCmdNull\r\n");
	at_backOk();
}

/**
 * @brief  Enable or disable Echo.
 * @param  id: command id number
 * @param  pPara:
 * @retval None
 */
void at_setupCmdE(tATNode* pNode, const char *pPara) {
	at_debug("at_setupCmdE:%d\r\n", *pPara - '0');
	if (*pPara == '0') {
		echoFlag = 0;
	} else if (*pPara == '1') {
		echoFlag = 1;
	} else {
		at_backError();
		return;
	}
	at_backOk();
}

/**
 * @brief  Execution commad of reset.
 * @param  id: commad id number
 * @retval None
 */
void at_exeCmdRst(tATNode* pNode) {
	at_debug("at_exeCmdRst\r\n");
	at_backOk();
	system_restart();
}

/**
 * @brief  Execution commad of version.
 * @param  id: commad id number
 * @retval None
 */
void at_exeCmdGmr(tATNode* pNode) {
	char temp[64];
	at_debug("at_exeCmdGmr\r\n");
	sprintf(temp, "-AT_VERSION:%02X-", AT_VERSION);
	at_response(temp);
	at_backOk();
}

