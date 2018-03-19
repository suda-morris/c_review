#include <stdio.h>
#include <stdlib.h>

#include "LinkList.h"

typedef struct Node {
	tLinkListNode *pNext;
	int data;
} tNode;

static int SearchStrategy(tLinkListNode *pNode, void *args) {
	if (pNode == NULL) {
		return -1;
	}
	tNode* childNode = (tNode*) pNode;
	if (childNode->data == *((int*) args)) {
		return 0;
	} else {
		return -1;
	}
}

static int DeleteStrategy(tLinkListNode *pNode, void *args) {
	if (pNode == NULL) {
		return -1;
	}
	tNode *childNode = (tNode*) pNode;
	if (childNode->data == *((int*) args)) {
		return 0;
	} else {
		return -1;
	}
}

static int InsertStrategy(tLinkListNode *pNode, void *args) {
	if (pNode == NULL) {
		return -1;
	}
	tNode *childNode = (tNode*) pNode;
	if (childNode->data == *((int*) args)) {
		return 0;
	} else {
		return -1;
	}
}

static void PrintStrategy(tLinkListNode *pNode) {
	if (pNode == NULL) {
		return;
	}
	tNode *childNode = (tNode*) pNode;
	printf("%d\n", childNode->data);
}

int main(void) {
	int i = 0;
	tLinkList *list = singleListInit();
	tNode* pNode = NULL;
	for (i = 0; i < 5; i++) {
		pNode = (tNode*) malloc(sizeof(tNode));
		pNode->pNext = NULL;
		pNode->data = i;
		list->addListNode(list, (tLinkListNode*) pNode);
	}
	list->printList(list, PrintStrategy);
	printf("Head:%d;	Tail:%d\r\n", ((tNode*) list->pHead)->data,
			((tNode*) list->pTail)->data);

	printf("--------------------------\n");
	int del = 2;
	list->delListNode(list, DeleteStrategy, &del);
	list->printList(list, PrintStrategy);
	printf("Head:%d;	Tail:%d\r\n", ((tNode*) list->pHead)->data,
			((tNode*) list->pTail)->data);

	printf("--------------------------\n");
	int sNum = 1;
	pNode = (tNode*) list->searchList(list, SearchStrategy, &sNum);
	if (pNode) {
		printf("search:%d\n", pNode->data);
		pNode = (tNode*) list->getListNextNode(list, (tLinkListNode*) pNode);
		if (pNode) {
			printf("next:%d\n", pNode->data);
		}
	}

	printf("--------------------------\n");
	pNode = (tNode*) malloc(sizeof(tNode));
	pNode->pNext = NULL;
	pNode->data = 88;
	list->insertListNode(list, (tLinkListNode*) pNode, InsertStrategy, &sNum);
	list->printList(list, PrintStrategy);
	printf("Head:%d;	Tail:%d\r\n", ((tNode*) list->pHead)->data,
			((tNode*) list->pTail)->data);

	printf("--------------------------\n");
	pNode = (tNode*) list->searchNodeM(list, 3);
	printf("%d\n", pNode->data);
	pNode = (tNode*) list->searchNodeMInverse(list, 3);
	printf("%d\n", pNode->data);

	printf("--------------------------\n");
	pNode = (tNode*) malloc(sizeof(tNode));
	pNode->data = 66;
	pNode->pNext = list->searchNodeM(list, 2);
	list->addListNode(list, (tLinkListNode*) pNode);
	list->printList(list, PrintStrategy);
	printf("Head:%d;	Tail:%d\r\n", ((tNode*) list->pHead)->data,
			((tNode*) list->pTail)->data);

	printf("--------------------------\n");
	sNum = 0;
	list->delListNode(list, DeleteStrategy, &sNum);
	list->printList(list, PrintStrategy);
	printf("Head:%d;	Tail:%d\r\n", ((tNode*) list->pHead)->data,
			((tNode*) list->pTail)->data);

	list->delLinkList(list);
	pNode = NULL;
	list = NULL;
	return EXIT_SUCCESS;
}
