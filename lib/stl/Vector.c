#include "Vector.h"
#include "Util.h"

Vector* Vector_New(unsigned int elem_size,unsigned int size)
{
	Vector* newVector = NULL;

	if (!elem_size){
		return NULL;
	}
	if (size <= 0){
		size = DEFALUT_VECTOR_SIZE;
	}
	newVector = (Vector*)xMalloc(sizeof(Vector));
	if (NULL == newVector){
		return NULL;
	}
	newVector->element = xMalloc(elem_size * size);
	if (NULL == newVector->element){
		xFree(newVector);
		newVector = NULL;
		return NULL;
	}
	newVector->size = 0;
	newVector->capacity = size;
	newVector->elem_size = elem_size;

	return newVector;
}

void Vector_Free(Vector **v)
{
	if (NULL != *v) {
		if ((*v)->element != NULL){
			xFree((*v)->element);
		}
		xFree(*v);
	}
}

int Vector_Enlarge(Vector *v)
{
	Element element = NULL;
	unsigned int capacity = 0;

	if (NULL == v){
		return -1;
	}
	
	capacity = v->capacity * 2;
	element = xRealloc(v->element, v->elem_size * capacity);

	if (element == NULL) {
		return 0;
	} else {
		v->capacity = capacity;
		v->element = element;
		return 1;
	}
}

void* Vector_At(Vector *v, unsigned int index)
{
	if( index < 0 || index > v->size ){
		return NULL;
	}
	return v->element + v->elem_size * index;
}

void* Vector_Front(Vector* v)
{
	if( v->size == 0 ){
		return NULL;
	}
	return v->element;
}

void* Vector_Back(Vector* v)
{
	if( v->size == 0 ){
		return NULL;
	}
	return v->element + ( v->size - 1 ) * v->elem_size ;
}

int Vector_Empty(Vector* v)
{
	if( v->size ){
		return 0;
	}
	return 1;
}
unsigned int Vector_Size(Vector* v)
{
	return v->size;
}

unsigned int Vector_Capacity(Vector* v)
{
	return v->capacity;
}

int Vector_Reserve(Vector* v, unsigned int new_cap)
{
	if( v->capacity >= new_cap ){
		return 0;
	}

	v->element = xRealloc(v->element, new_cap * v->elem_size );
	if( NULL == v->element ){
		return -1;
	}
	return 0;
}

void Vector_Clear(Vector* v)
{
	xMemSet( v->element, 0 , v->elem_size * v->size);
}

int Vector_Insert(Vector* v,void *elem, unsigned int index)
{
	unsigned int pos = 0;
	unsigned int i = 0;

	if( v->size + 1 >= v->capacity ){
		Vector_Enlarge(v);
	}
	if( index <= 0 ){
		pos = 0;
	}
	else if( index >= v->size ){
		pos = v->size;
	}
	else{
		pos = index;
	}

	for( i = v->size; i > pos ; --i){
		*(v->element + v->elem_size * i) = *(v->element + v->elem_size * (i-1) );
	}
	xMemCpy(v->element + v->elem_size * pos, elem, v->elem_size);
	v->size++;
	return 0;
}

int Vector_Erase(Vector* v, unsigned int index)
{
	unsigned int pos = 0;
	unsigned int i = 0;

	if( index <= 0 ){
		pos = 0;
	}
	else if( index >= v->size){
		pos = v->size;
	}
	else{
		pos = index;
	}
	for( i = pos ; i < v->size - 1 ; ++i ){
		*(v->element + v->elem_size * i) = *( v->element + v->elem_size * (i+1));
	}
	xMemSet( v->element + v->elem_size * v->size  , 0 , v->elem_size);
	v->size--;
	return 0;
}
int Vector_Push_Back(Vector* v,void *elem)
{
	 return Vector_Insert(v,elem,v->size);
}

int Vector_Append(Vector* v,void *elem)
{
	return Vector_Push_Back(v,elem);
}

int Vector_Pop_Back(Vector* v,void *elem)
{
	if( 0 == v->size || NULL == v){
		return -1;
	}
	xMemCpy(elem, v->element + v->elem_size * v->size , v->elem_size);
	xMemSet(v->element + v->elem_size * v->size , 0 , v->elem_size);
	v->size--;
	return 0;
}

int Vector_Resize(Vector* v)
{
	if( v->capacity * 2/3 > v->size ){
		v->element = xRealloc(v->element, 2/3 * v->capacity * v->elem_size);
	}
	return 0;
}

int Vector_Equal(Vector* v1,Vector* v2)
{
	if( v1->size != v2->size ){
		return -1;
	}
	return xMemCmp(v1->element,v2->element,v1->size);
}
