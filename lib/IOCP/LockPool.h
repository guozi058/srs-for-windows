#ifndef _LOCKPOOL_H_
#define _LOCKPOOL_H_

#include "Lock.h"

typedef struct LOCKPOOL_T
{
	unsigned int size;
	Lock** lockList;
	unsigned int index;

}LockPool;

#ifdef __cplusplus
extern "C"
{
#endif

LockPool* LockPool_New();
int LockPool_Destroy(LockPool*);
int LockPool_Init(LockPool*,unsigned int);
Lock* LockPool_Allocate(LockPool*);
void LockPool_Recycle(Lock*);

#ifdef __cplusplus
}
#endif

#endif
