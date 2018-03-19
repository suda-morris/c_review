#include <string.h>
#include <stdlib.h>
#include "ATcmd.h"
#include "LinkList.h"
#include "AT_BaseCmd.h"

#define AT_BASICCMDNUM   	4

static tLinkList *at_linkList = NULL;
static at_funcationType at_basicfun[AT_BASICCMDNUM] =
		{ { NULL, 0, NULL, NULL,
		NULL, at_exeCmdNull }, { "E", 1, NULL, NULL, at_setupCmdE, NULL }, {
				"+RST", 4,
				NULL, NULL, NULL, at_exeCmdRst }, { "+GMR", 4, NULL, NULL, NULL,
				at_exeCmdGmr } };

uint8_t echoFlag = 1;
void (*system_restart)(void) = NULL;

/**
 * @brief  Get the length of commad.
 * @param  pCmd: point to received command
 * @retval the length of command, 'A'and'T' excluded.
 *   @arg -1: failure
 */
static int8_t at_getCmdLen(const char *pCmd) {
	uint8_t n, i;
	n = 0;
	i = 128;
	while (i--) {
		if ((*pCmd == '\r') || (*pCmd == '=') || (*pCmd == '?')
				|| ((*pCmd >= '0') && (*pCmd <= '9'))) {
			return n;
		} else {
			pCmd++;
			n++;
		}
	}
	return -1;
}

/**
 * @brief  Query and localization one commad.
 * @param  cmdLen: received length of command
 * @param  pCmd: point to received command
 * @retval the node address searched or NULL
 * 	@arg NULL: failure
 */
static tATNode* at_cmdSearch(int8_t cmdLen, const char *pCmd) {
	int16_t i;
	tATNode* pNode = (tATNode*) at_linkList->pHead;
	if (cmdLen == 0) {
		return pNode;
	} else if (cmdLen > 0) {
		pNode = (tATNode*) pNode->pNext;
		for (i = 1; i < at_linkList->SumOfNode; i++) {
			if (cmdLen == pNode->at_fun->at_cmdLen) {
				if (strncmp((const char*) pCmd,
						(const char*) pNode->at_fun->at_cmdName, cmdLen) == 0) {
					return pNode;
				}
			}
			pNode = (tATNode*) pNode->pNext;
		}
	}
	return NULL;
}

/**
 * @brief  Distinguish commad and to execution.
 * @param  pAtRcvData: point to received (command)
 * @retval None
 */
void at_cmdProcess(const char *pAtRcvData) {
	uint8_t cmdLen;
	tATNode *pNode = NULL;

	cmdLen = at_getCmdLen(pAtRcvData);
	if (cmdLen != -1) {
		pNode = at_cmdSearch(cmdLen, pAtRcvData);
	} else {
		pNode = NULL;
	}
	if (pNode != NULL) {
		pAtRcvData += cmdLen;
		if (*pAtRcvData == '\r') {
			if (pNode->at_fun->at_exeCmd) {
				pNode->at_fun->at_exeCmd(pNode);
			} else {
				at_backError();
			}
		} else if (*pAtRcvData == '?' && (pAtRcvData[1] == '\r')) {
			if (pNode->at_fun->at_queryCmd) {
				pNode->at_fun->at_queryCmd(pNode);
			} else {
				at_backError();
			}
		} else if ((*pAtRcvData == '=') && (pAtRcvData[1] == '?')
				&& (pAtRcvData[2] == '\r')) {
			if (pNode->at_fun->at_testCmd) {
				pNode->at_fun->at_testCmd(pNode);
			} else {
				at_backError();
			}
		} else if (((*pAtRcvData >= '0') && (*pAtRcvData <= '9'))
				|| (*pAtRcvData == '=')) {
			if (pNode->at_fun->at_setupCmd) {
				pNode->at_fun->at_setupCmd(pNode, (const char*) pAtRcvData);
			} else {
				at_backError();
			}
		} else {
			at_backError();
		}
	} else {
		at_backError();
	}
}

/**
 * @brief  register system restart function
 * @param  sys_restart: point to restart function implement by user
 * @retval None
 */
void at_regCallback(void (*sys_restart)(void)) {
	system_restart = sys_restart;
}

/**
 * @brief  register user command
 * @param  new_atcmd: point to a user command
 * @retval None
 */
void at_regCommand(at_funcationType *new_atcmd) {
	tATNode* pNode = (tATNode*) malloc(sizeof(tATNode));
	pNode->at_fun = new_atcmd;
	pNode->pNext = NULL;
	at_linkList->addListNode(at_linkList, (tLinkListNode*) pNode);
}

void at_init(void) {
	int i = 0;
	tATNode* pNode = NULL;
	at_linkList = singleListInit();
	for (i = 0; i < AT_BASICCMDNUM; i++) {
		pNode = (tATNode*) malloc(sizeof(tATNode));
		pNode->pNext = NULL;
		pNode->at_fun = &at_basicfun[i];
		at_linkList->addListNode(at_linkList, (tLinkListNode*) pNode);
	}
}

void at_destory() {
	at_linkList->delLinkList(at_linkList);
}
