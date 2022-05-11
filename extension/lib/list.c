// list.c
// Dustin Mierau

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "list.h"

ListNode* ListNodeCreate(void) {
	ListNode* node = malloc(sizeof(ListNode));
	node->data = NULL;
	node->next = NULL;
	return node;
}

void ListNodeDestroy(ListNode* node) {
	free(node);
}

List* ListCreate(void) {
	List* list = malloc(sizeof(List));
	list->first = ListNodeCreate();
	return list;
}

void ListDestroy(List* list) {
	ListNode* node = list->first;
	ListNode* next;
	while(node != NULL) {
		next = node->next;
		free(node);
		node = next;
	}
	free(list);
}

void ListDestroyAll(List* list) {
	ListNode* node = list->first;
	ListNode* next;
	while(node != NULL) {
		next = node->next;
		if(node->data != NULL) {
			free(node->data);
			node->data = NULL;
		}
		free(node);
		node = next;
	}
	free(list);
}

void ListAppend(List* list, void* data) {
	ListNode* node = list->first;
	while(node->next != NULL) {
		node = node->next;
	}
	node->data = data;
	node->next = ListNodeCreate();
}

void ListInsert(List* list, int index, void* data) {
	if(index == 0) {
		ListNode* after = list->first;
		list->first = ListNodeCreate();
		list->first->data = data;
		list->first->next = after;
		return;
	}
	else if(index == ListGetLength(list)) {
		ListAppend(list, data);
		return;
	}

	ListNode* before = list->first;
	ListNode* after = list->first->next;
	while(index > 1) {
		index--;
		before = before->next;
		after = after->next;
	}
	before->next = ListNodeCreate();
	before->next->data = data;
	before->next->next = after;
}

void* ListGet(List* list, int index) {
	ListNode* node = list->first;
	while(index > 0) {
		node = node->next;
		index--;
	}
	return node->data;
}

void* ListRemove(List* list, int index) {
	if(index == 0) {
		ListNode* node = list->first;
		list->first = list->first->next;
		void* data = node->data;
		ListNodeDestroy(node);
		return data;
	}
	
	ListNode* before = list->first;
	while(index > 1) {
		before = before->next;
		index--;
	}
	ListNode* node = before->next;
	before->next = before->next->next;
	void* data = node->data;
	ListNodeDestroy(node);
	return data;
}

int ListGetLength(List* list) {
	ListNode* node = list->first;
	int length = 0;
	while(node->next != NULL) {
		length++;
		node = node->next;
	}
	return length;
}

