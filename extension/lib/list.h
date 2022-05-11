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

extern List* ListCreate(void);
extern void ListDestroy(List* list);
extern void ListDestroyAll(List* list);
extern void ListAppend(List* list, void* data);
extern void ListInsert(List* list, int index, void* data);
extern void* ListGet(List* list, int index);
extern void* ListRemove(List* list, int index);
extern int ListGetLength(List* list);

#endif