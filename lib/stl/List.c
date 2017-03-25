#include "List.h"
#include "Util.h"

int List_Create(LIST **list,unsigned int elem_size)
{
	if( NULL == list){
		return -1;
	}

	(*list) = xMalloc( sizeof(LIST) );
	if( NULL == (*list)){
		return -1;
	}

	(*list)->size = 0;
	(*list)->elem_size = elem_size;
	(*list)->head = xMalloc(sizeof(LIST_NODE*) );

	if( NULL == (*list)->head ){
		xFree((*list));
		return -1;
	}
	return 0;
}

LIST* List_New(unsigned int elem_size)
{
	LIST* list = NULL;
	list = xMalloc(sizeof(LIST));
	if( NULL == list){
		return NULL;
	}

	list->size = 0;
	list->elem_size = elem_size;
	list->head = xMalloc(sizeof(LIST_NODE*));

	if( NULL == list->head ){
		xFree((list));
		return NULL;
	}
	return list;
}


int List_Free(LIST **list)
{
	LIST_NODE *tmp = NULL;
	LIST_NODE *ptr = NULL;

	if( NULL == (*list)){
		return -1;
	}

	 ptr = (*list)->head;

	while( ptr != NULL ){
		tmp = ptr;
		ptr = ptr->next;
		xFree( tmp->element );
		xFree( tmp );
	}
	xFree( *list );
	return 0;
}

void* List_Front(LIST *list)
{
	if( NULL == list ){
		return NULL;
	}
	if( NULL != list->head ){
		return list->head;
	}
	return NULL;
}

void* List_Back(LIST *list)
{
	LIST_NODE *ptr = NULL;
	if( NULL == list ){
		return NULL;
	}
	ptr = list->head;

	while( ptr->next != NULL ){
		ptr = ptr->next;
	}
	//return ptr->element;
	return ptr;
}

int List_Empty(LIST *list){
	if( NULL == list ){
		return -1;
	}
	if( 0 == list->size ){
		return 0;
	}
	return 1;
}

unsigned int List_Size(LIST *list){
	if( NULL == list ){
		return 0;
	}
	return list->size;
}

void List_Clear(LIST *list)
{
	LIST_NODE *ptr = NULL;
	LIST_NODE *tmp = NULL;

	if( NULL == list ){
		return;
	}
	ptr = list->head;

	while( ptr != NULL ){
		tmp = ptr;
		ptr = ptr->next;
		xFree(tmp->element);
		xFree(tmp);
	}
}

void List_Insert(LIST *list,void *elem, unsigned int index)
{
	unsigned int counter = 0;
	LIST_NODE *ptr = NULL;
	LIST_NODE *ptr2 = NULL;
	LIST_NODE *tmp = NULL;

	if( NULL == list || NULL == elem ){
		return ;
	}

	ptr = list->head;

	while( ptr != NULL && counter < index ){
		ptr2 = ptr;
		ptr = ptr->next;
		counter ++;
	}
	ptr = ptr2;

	tmp = xMalloc( sizeof(LIST_NODE ));
	tmp->element = xMalloc( list->elem_size );
	xMemCpy(tmp->element,elem,list->elem_size);

	if( list->head == ptr ){
		ptr->prev  = tmp;
		tmp->next  = ptr;
		list->head = tmp;
		tmp->prev  = NULL;
	}
	else if( NULL == ptr->next ){
		ptr->next = tmp;
		tmp->prev = ptr;
		tmp->next = NULL;
	}
	else{
		tmp->prev = ptr->prev;
		tmp->next = ptr;
		tmp->prev->next = tmp;
		ptr->prev = tmp;
	}
	list->size += 1;
}

void List_Erase(LIST *list,unsigned int index)
{
	unsigned int counter = 0;
	LIST_NODE *ptr = NULL;
	LIST_NODE *ptr2 = NULL;

	if( NULL == list ){
		return;
	}

	ptr = list->head;

	while( ptr != NULL && counter < index){
		ptr2 = ptr;
		ptr  = ptr->next;
		counter += 1;
	}
	ptr = ptr2;

	if( ptr->next == ptr ){
		ptr->next = ptr->next;
		ptr->next->prev = NULL;
		xFree( ptr->element );
		xFree( ptr );
	}
	else if( NULL == ptr->next ){
		ptr->prev->next = NULL;
		xFree( ptr->element );
		xFree( ptr);
	}
	else{
		ptr->prev->next = ptr->next;
		ptr->next->prev = ptr->prev;
		xFree(ptr->element);
		xFree(ptr);
	}
	list->size -= 1;
}

void List_Push_Back(LIST *list, void *elem){
	List_Insert(list,elem,list->size);
}

void List_Pop_Back(LIST *list, void *elem)
{
	LIST_NODE* ptr = NULL;
	LIST_NODE* ptr2 = NULL;

	if( NULL == list || NULL == elem ){
		return;
	}

	ptr = list->head;

	while( ptr != NULL ){
		ptr2 = ptr;
		ptr  = ptr->next;
	}
	ptr = ptr2;

	xMemCpy(elem,ptr->element,list->elem_size);
	xFree(ptr->element);
	xFree(ptr);
	list->size -= 1;
}

void List_Push_Front(LIST *list, void *elem)
{
	LIST_NODE *ptr = NULL;
	LIST_NODE *tmp = NULL;

	if( NULL == list || NULL == elem){
		return;
	}
	ptr = list->head;
	tmp = xMalloc( sizeof(LIST_NODE ));
	if( NULL == tmp ){
		return ;
	}
	tmp->element = xMalloc( list->elem_size );
	if( NULL == tmp->element ){
		xFree(tmp);
		return;
	}
	xMemCpy(tmp->element,elem,list->elem_size);

	if( NULL == ptr ){
		list->head = tmp;
		tmp->next  = NULL;
		tmp->prev  = NULL;
	}
	else{
		ptr->prev  = tmp;
		tmp->next  = ptr;
		list->head = tmp;
		tmp->prev  = NULL;
	}
	list->size += 1;
}

void List_Pop_Front(LIST *list,void *elem)
{
	LIST_NODE *ptr = NULL;

	if( NULL == list || NULL == elem){
		return;
	}
	ptr = list->head;

	if( NULL == ptr){
		return;
	}

	if( NULL == ptr->next ){
		xMemCpy(elem,ptr->element,list->elem_size);
		list->head = NULL;
		xFree(ptr->element);
		xFree(ptr);
	}
	else{
		xMemCpy(elem,ptr->element,list->elem_size);
		list->head      = ptr->next;
		ptr->next->prev = NULL;
		xFree(ptr->element);
		xFree(ptr);
	}
	list->size -= 1;
}

void* List_At(LIST *list, unsigned int index)
{
	LIST_NODE* ptr = NULL;
	LIST_NODE* node = NULL;
	unsigned int counter = 0;

	if( NULL == list || !list->size){
		return NULL;
	}
	if (index >= list->size){
		return NULL;
	}

	ptr = list->head;

	while( ptr != NULL && counter <= index ){
		node = ptr;
		ptr = ptr->next;
		counter++;
	}
	return node;
}

int List_Remove(LIST* list, void *data)
{
	LIST_NODE *ptr = NULL;
	LIST_NODE *node = NULL;
	unsigned int found = FALSE;

	if( NULL == list || !list->size){
		return -1;
	}

	ptr = list->head;

	while( ptr != NULL){
		node = ptr;
		if (!xMemCmp(node->element, data, list->elem_size)){
			node->prev->next = node->next;
			node->next->prev = node->prev;
			xFree(node->element);
			xFree(node);
			found = TRUE;
			break;
		}
		ptr = ptr->next;
	}
	if (!found){
		return -1;
	}
	list->size -= 1;
	return 0;
}