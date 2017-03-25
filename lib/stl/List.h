#ifndef _LIST_H_
#define _LIST_H_

typedef struct LIST_NODE_T
{
	struct LIST_NODE_T *next;
	struct LIST_NODE_T *prev;
	void* element;
}LIST_NODE;

typedef struct LIST_T{
	unsigned int size;
	unsigned int elem_size;
	LIST_NODE *head;
}LIST;

#ifdef __cplusplus
extern "C"
{
#endif

int List_Create(LIST **list,unsigned int elem_size);
LIST* List_New(unsigned int elem_size);
int List_Free(LIST **list);
void* List_Front(LIST *list);
void* List_Back(LIST *list);
int List_Empty(LIST *list);
unsigned int List_Size(LIST *list);
void List_Clear(LIST *list);
void List_Insert(LIST *list,void *elem, unsigned int index);
void List_Erase(LIST *list,unsigned int index);
void List_Push_Back(LIST *list, void *elem);
void List_Pop_Back(LIST *list, void *elem);
void List_Push_Front(LIST *list, void *elem);
void List_Pop_Front(LIST *list,void *elem);

void* List_At(LIST *list, unsigned int index);
int List_Remove(LIST* list, void *data);

#ifdef __cplusplus
}
#endif

#endif