#ifndef _VECTOR_H
#define _VECTOR_H

#define DEFALUT_VECTOR_SIZE 16

typedef void* Element;

typedef struct VECTOR_T
{
	unsigned int elem_size;
	unsigned int size;
	unsigned int capacity;
	unsigned char* element;
}Vector;

Vector* Vector_New(unsigned int elem_size,unsigned int size);
void Vector_Free(Vector **v);

int Vector_Enlarge(Vector *v);
void* Vector_At(Vector *v, unsigned int index);
void* Vector_Front(Vector* v);
void* Vector_Back(Vector* v);
int Vector_Empty(Vector* v);
unsigned int Vector_Size(Vector* v);
unsigned int Vector_Capacity(Vector* v);
int Vector_Reserve(Vector* v, unsigned int new_cap);
void Vector_Clear(Vector* v);
int Vector_Insert(Vector* v,void *elem, unsigned int index);
int Vector_Erase(Vector* v, unsigned int index);
int Vector_Push_Back(Vector* v,void *elem);
int Vector_Append(Vector* v,void *elem);
int Vector_Pop_Back(Vector* v,void *elem);
int Vector_Resize(Vector* v);
int Vector_Equal(Vector* v1,Vector* v2);

#endif
