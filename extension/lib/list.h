// list.h
// Dustin Mierau

#ifndef list_h
#define list_h

typedef struct _ListNode {
	void* data;
	struct _ListNode* next;
} ListNode;

extern ListNode* ListNodeCreate(void);
extern void ListNodeDestroy(ListNode* node);

typedef struct _List {
	struct _ListNode* first;
} List;

List* ListCreate(void);
void ListDestroy(List* list);
void ListDestroyAll(List* list);
void ListAppend(List* list, void* data);
void ListInsert(List* list, int index, void* data);
void* ListGet(List* list, int index);
void* ListRemove(List* list, int index);
int ListGetLength(List* list);

#endif