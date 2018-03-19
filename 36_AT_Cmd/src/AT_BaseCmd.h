/*
 * AT_BaseCmd.h
 *
 *  Created on: 2015年12月11日
 *      Author: morris
 */

#ifndef AT_BASECMD_H_
#define AT_BASECMD_H_
#include <stdint.h>
#include "ATcmd.h"
void at_exeCmdNull(tATNode* pNode);
void at_setupCmdE(tATNode* pNode, const char *pPara);
void at_exeCmdRst(tATNode* pNode);
void at_exeCmdGmr(tATNode* pNode);

#endif /* AT_BASECMD_H_ */
