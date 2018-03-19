/*
 *  LinkList.c
 *
 *  Created on: 2015年3月10日
 *      Author: morris
 */

#include "LinkList.h"

#include <stdlib.h>

/**
 * @brief  add a node to the end of linklist
 * @param  pList: point to LinkList Object
 * @param  pListNode: point to the node to be added
 * @retval result of add list node
 *   @arg -1: failure
 *   @arg 0: success
 */
static int _AddListNode(tLinkList *pList, tLinkListNode *pListNode) {
	if (pList == NULL || pListNode == NULL) {
		return -1;
	}
	if (pList->pTail == NULL) {
		pList->pTail = pListNode;
	} else {
		pList->pTail->pNext = pListNode;
		pList->pTail = pListNode;
	}
	pList->SumOfNode++;
	if (pList->SumOfNode == 1) {
		pList->pHead = pListNode;
	}
	return 0;
}

/**
 * @brief  Search Node In List By Condition Given By User
 * @param  pList: point to LinkList Object
 * @param  SearchCondition: a func to check if it is the wanted node to search,if passed,return 0
 * @param  args: argument transferd to SearchCondition func
 * @retval pointer of the Searched Node or NULL
 */
static tLinkListNode* _SearchList(tLinkList *pList,
		int (*SearchCondition)(tLinkListNode *pNode, void *args), void *args) {
	tLinkListNode* circleNode = NULL;
	tLinkListNode* pNode = NULL;
	if (pList == NULL || SearchCondition == NULL) {
		return NULL;
	}
	circleNode = pList->checkCircle(pList);
	pNode = pList->pHead;
	while (pNode != circleNode) {
		if (SearchCondition(pNode, args) == 0) {
			return pNode;
		}
		pNode = pNode->pNext;
	}
	if (circleNode == NULL) {
		return NULL;
	} else {
		if (SearchCondition(pNode, args) == 0) {
			return pNode;
		}
		pNode = pNode->pNext;
		while (pNode != circleNode) {
			if (SearchCondition(pNode, args) == 0) {
				return pNode;
			}
			pNode = pNode->pNext;
		}
	}
	return NULL;
}

/**
 * @brief  Get Next Node From List
 * @param  pList: point to LinkList Object
 * @param  pListNode: the previous node of the wanted node
 * @retval pointer of the next node or NULL
 */
static tLinkListNode* _GetListNextNode(tLinkList *pList,
		tLinkListNode *pListNode) {
	tLinkListNode* pNode = NULL;
	tLinkListNode* circleNode = NULL;
	if (pList == NULL || pListNode == NULL) {
		return NULL;
	}
	circleNode = pList->checkCircle(pList);
	pNode = pList->pHead;
	while (pNode != circleNode) {
		if (pNode == pListNode) {
			return pNode->pNext;
		} else {
			pNode = pNode->pNext;
		}
	}
	if (circleNode == NULL) {
		return NULL;
	} else {
		if (pNode == pListNode) {
			return pNode->pNext;
		} else {
			pNode = pNode->pNext;
		}
		while (pNode != circleNode) {
			if (pNode == pListNode) {
				return pNode->pNext;
			} else {
				pNode = pNode->pNext;
			}
		}
	}
	return NULL;
}

/**
 * @brief  insert a Node into LinkList
 * @param  pList: point to LinkList Object
 * @param  pListNode: point to the node to be added
 * @param  InsertCondition: a func to check whether to insert a node,if passed return 0
 * @param  args: argument transferd to DeleteCondition func
 * @retval result of Insert list node
 *   @arg -1: failure
 *   @arg 0: success
 */
static int _InsertListNode(tLinkList *pList, tLinkListNode *pListNode,
		int (*InsertCondition)(tLinkListNode *pNode, void *args), void *args) {
	if (pList == NULL || InsertCondition == NULL) {
		return -1;
	}
	tLinkListNode *pNode = _SearchList(pList, InsertCondition, args);
	if (pNode) {
		pListNode->pNext = pNode->pNext;
		pNode->pNext = pListNode;
		pList->SumOfNode++;
		if (pNode == pList->pTail) {
			pList->pTail = pListNode;
		}
		return 0;
	} else {
		return -1;
	}
}

/**
 * @brief  Delete a Node from LinkList
 * @param  pList: point to LinkList Object
 * @param  DeleteCondition: a func to check whether to delete a node,if passed return 0
 * @param  args: argument transferd to DeleteCondition func
 * @retval result of Delete list node
 *   @arg -1: failure
 *   @arg 0: success
 */
static int _DeleteListNode(tLinkList *pList,
		int (*DeleteCondition)(tLinkListNode *pNode, void *args), void *args) {
	tLinkListNode* pNode = NULL;
	tLinkListNode* circleNode = NULL;
	tLinkListNode* pTemp = NULL;
	tLinkListNode* ppTemp = NULL;
	if (pList == NULL || pList->SumOfNode == 0 || DeleteCondition == NULL) {
		return -1;
	}
	circleNode = pList->checkCircle(pList);
	pNode = pList->pHead;
	if (DeleteCondition(pNode, args) == 0) {
		//delete the head
		if (pNode != circleNode) {
			pList->pHead = pNode->pNext;
			pList->SumOfNode--;
			if (pList->SumOfNode == 0) {
				pList->pTail = NULL;
			}
			free(pNode);
			return 0;
		} else {
			pTemp = pNode;
			while (pNode->pNext != circleNode) {
				pNode = pNode->pNext;
			}
			pNode->pNext = pTemp->pNext;
			pList->pHead = pTemp->pNext;
			pList->SumOfNode--;
			if (pList->SumOfNode == 0) {
				pList->pTail = NULL;
				pList->pHead = NULL;
			}
			free(pTemp);
			return 0;
		}
	}
	while (pNode->pNext != circleNode) {
		if (DeleteCondition(pNode->pNext, args) == 0) {
			pTemp = pNode->pNext;
			if (pTemp == pList->pTail) {
				pList->pTail = pNode;
			}
			pNode->pNext = pTemp->pNext;
			free(pTemp);
			pList->SumOfNode--;
			return 0;
		} else {
			pNode = pNode->pNext;
		}
	}
	if (circleNode == NULL) {
		return -1;
	} else {
		if (DeleteCondition(pNode->pNext, args) == 0) {
			ppTemp = pNode;
			pNode = pNode->pNext;
			while (pNode->pNext != circleNode) {
				pNode = pNode->pNext;
			}
			pTemp = pNode;
			ppTemp->pNext = circleNode->pNext;
			pTemp->pNext = circleNode->pNext;
			free(circleNode);
			pList->SumOfNode--;
			return 0;
		} else {
			pNode = pNode->pNext;
			while (pNode->pNext != circleNode) {
				if (DeleteCondition(pNode->pNext, args) == 0) {
					pTemp = pNode->pNext;
					pNode->pNext = pTemp->pNext;
					free(pTemp);
					pList->SumOfNode--;
					if (pNode->pNext == circleNode) {
						pList->pTail = pNode;
					}
					return 0;
				} else {
					pNode = pNode->pNext;
				}
			}
			return -1;
		}
	}
}

/**
 * @brief search single link list for the No.M node start from the head
 * @param pList: point to LinkList Object
 * @param m: order number
 * @retval pointer to the searched node or NULL
 */
static tLinkListNode* _SearchNodeM(tLinkList* pList, uint32_t m) {
	tLinkListNode *pNode = NULL;
	uint32_t i = 0;
	if (pList == NULL || m <= 0 || m > (pList->SumOfNode)) {
		return NULL;
	}
	pNode = pList->pHead;
	if (pNode != NULL) {
		for (i = 0; i < m - 1; i++) {
			pNode = pNode->pNext;
			if (pNode == NULL) {
				return NULL;
			}
		}
	} else {
		return NULL;
	}
	return pNode;
}

/**
 * @brief search single link list for the No.M node start from the bottom
 * @param pList: point to LinkList Object
 * @param m: order number
 * @retval pointer to the searched node or NULL
 */
static tLinkListNode* _SearchNodeMInverse(tLinkList* pList, uint32_t m) {
	tLinkListNode *pNode = NULL;
	tLinkListNode *qNode = NULL;
	uint32_t i = 0;
	if (pList == NULL || m <= 0 || m > (pList->SumOfNode)) {
		return NULL;
	}
	pNode = pList->pHead;
	if (pNode != NULL) {
		for (i = 0; i < m - 1; i++) {
			pNode = pNode->pNext;
			if (pNode == NULL) {
				return NULL;
			}
		}
		qNode = pList->pHead;
		while (pNode != pList->pTail) {
			pNode = pNode->pNext;
			qNode = qNode->pNext;
		}
	} else {
		return NULL;
	}
	return qNode;
}

/**
 * @brief check whether there is a circle in the single link list
 * @param pList: point to LinkList Object
 * @retval pointer to the circle start node or NULL
 */
static tLinkListNode* _CheckCircle(tLinkList* pList) {
	tLinkListNode *pNode = NULL;
	tLinkListNode *qNode = NULL;
	if (pList == NULL) {
		return NULL;
	}
	pNode = qNode = pList->pHead;
	while (qNode != NULL && qNode->pNext != NULL) {
		pNode = pNode->pNext;
		qNode = qNode->pNext->pNext;
		if (pNode == qNode) {
			break;
		}
	}
	if (pNode == qNode && pNode != NULL) {
		qNode = pList->pHead;
		while (pNode != qNode) {
			pNode = pNode->pNext;
			qNode = qNode->pNext;
		}
		return pNode;
	} else {
		return NULL;
	}
}

/**
 * @brief  Print a LinkList
 * @param  pList: point to LinkList Object
 * @param  doprintlist: point to function which do print work
 */
static void _PrintList(tLinkList* pList,
		void (*doprintlist)(tLinkListNode* pNode)) {
	tLinkListNode* circleNode = NULL;
	tLinkListNode* pNode = NULL;
	if (pList == NULL) {
		return;
	}
	circleNode = _CheckCircle(pList);
	pNode = pList->pHead;
	while (pNode != circleNode) {
		if (pNode == NULL) {
			return;
		}
		doprintlist(pNode);
		pNode = pNode->pNext;
	}
	if (circleNode == NULL) {
		return;
	} else {
		doprintlist(pNode);
		pNode = pNode->pNext;
		while (pNode != circleNode) {
			doprintlist(pNode);
			pNode = pNode->pNext;
		}
	}
}

/**
 * @brief  Delete a LinkList
 * @param  pList: point to LinkList Object
 * @retval result of delete list
 *   @arg -1: failure
 *   @arg 0: success
 */
static int _DeleteList(tLinkList *pList) {
	tLinkListNode* circleNode = NULL;
	tLinkListNode* pNode = NULL;
	tLinkListNode *pTemp = NULL;
	if (pList == NULL) {
		return -1;
	}

	circleNode = _CheckCircle(pList);
	pNode = pList->pHead;
	pTemp = pNode;
	while (pNode != circleNode) {
		if (pNode == NULL) {
			return 0;
		}
		pTemp = pNode;
		pNode = pNode->pNext;
		free(pTemp);
	}
	if (circleNode == NULL) {
		return 0;
	} else {
		pNode = pNode->pNext;
		while (pNode != circleNode) {
			pTemp = pNode;
			pNode = pNode->pNext;
			free(pTemp);
		}
		free(pNode);
	}
	free(pList);
	return 0;
}

/**
 * @brief  Init Single Linked List Object
 * @retval pointer of the linklist object or NULL
 */
tLinkList* singleListInit() {
	tLinkList *list = (tLinkList*) malloc(sizeof(tLinkList));
	if (list == NULL) {
		return NULL;
	}

	list->SumOfNode = 0;
	list->pHead = NULL;
	list->pTail = NULL;
	list->delLinkList = _DeleteList;
	list->addListNode = _AddListNode;
	list->delListNode = _DeleteListNode;
	list->searchList = _SearchList;
	list->getListNextNode = _GetListNextNode;
	list->insertListNode = _InsertListNode;
	list->searchNodeM = _SearchNodeM;
	list->searchNodeMInverse = _SearchNodeMInverse;
	list->checkCircle = _CheckCircle;
	list->printList = _PrintList;

	return list;
}

/**
 * @brief  Init Single Linked List Object
 * @param pListA,pListB:single link list object to be checked
 * @retval
 * 	@arg:0 not cross
 * 	@arg:1 cross
 */
uint8_t isLinkListCross(tLinkList* pListA, tLinkList* pListB) {
	tLinkListNode *ANode = NULL;
	tLinkListNode *BNode = NULL;
	tLinkListNode *AcircleNode = NULL;
	tLinkListNode *BcircleNode = NULL;
	if (pListA == NULL || pListB == NULL) {
		return 0;
	}
	ANode = pListA->pHead;
	BNode = pListB->pHead;
	AcircleNode = _CheckCircle(pListA);
	BcircleNode = _CheckCircle(pListB);
	while (ANode != AcircleNode) {
		ANode = ANode->pNext;
	}
	while (BNode != BcircleNode) {
		BNode = BNode->pNext;
	}
	if (ANode == BNode) {
		return 1;
	} else {
		return 0;
	}
}
