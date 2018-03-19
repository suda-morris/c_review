/*
 * LinkList.h
 *
 *  Created on: 2015年3月10日
 *      Author: morris
 *      version: v0.1
 */
#ifndef LINKLIST_H_
#define LINKLIST_H_
#include <stdint.h>
/*
 * SingleLinkedList Node Type
 */
typedef struct LinkListNode {
	struct LinkListNode *pNext;
} tLinkListNode;

/*
 * SingleLinkedList Type
 */
typedef struct LinkList {
	tLinkListNode *pHead;
	tLinkListNode *pTail;
	int SumOfNode;
	int (*delLinkList)(struct LinkList *pList);
	int (*addListNode)(struct LinkList *pList, tLinkListNode *pListNode);
	int (*delListNode)(struct LinkList *pList,
			int (*DeleteCondition)(tLinkListNode *pNode, void *args),
			void *args);
	tLinkListNode* (*searchList)(struct LinkList *pList,
			int (*SearchCondition)(tLinkListNode *pNode, void *args),
			void *args);
	tLinkListNode* (*getListNextNode)(struct LinkList *pList,
			tLinkListNode *pListNode);
	int (*insertListNode)(struct LinkList *pList, tLinkListNode *pListNode,
			int (*InsertCondition)(tLinkListNode *pNode, void *args),
			void *args);
	tLinkListNode* (*searchNodeM)(struct LinkList* pList, uint32_t m);
	tLinkListNode* (*searchNodeMInverse)(struct LinkList* pList, uint32_t m);
	tLinkListNode* (*checkCircle)(struct LinkList* pList);
	void (*printList)(struct LinkList* pList,
			void (*doprintlist)(tLinkListNode* pNode));
} tLinkList;

/*
 * Init Single Linked List Object
 */
tLinkList* singleListInit();

/*
 * check to judge whether two single link list cross
 * if cross, return 1;else return 0
 */
uint8_t isLinkListCross(tLinkList* pListA, tLinkList* pListB);
#endif /* LINKLIST_H_ */
