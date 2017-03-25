#ifndef _LOCK_H_
#define _LOCK_H_

#include "Util.h"

typedef enum {
	LOCK_IDLE,
	LOCK_R,
	LOCK_W
}LOCK_STATE;

typedef struct LOCK_T
{
	int state;
	int readLockCount;
	int readWaitingCount;
	int writeWaitingCount;
	HANDLE event;
	CRITICAL_SECTION cs;
}Lock;


#ifdef __cplusplus
extern "C"
{
#endif

Lock* Lock_New();
void Lock_Init(Lock* lock);
void Lock_Destory(Lock* lock, unsigned int flag);
void Lock_Read(Lock* lock);
void Lock_Write(Lock* lock);
void Lock_Unlock(Lock* lock);

#ifdef __cplusplus
}
#endif

#endif
