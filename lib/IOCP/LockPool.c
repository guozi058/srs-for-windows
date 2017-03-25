#include "LockPool.h"

LockPool* LockPool_New()
{
	LockPool* lockpool = NULL;
	lockpool = xMalloc(sizeof(LockPool));
	if (NULL == lockpool){
		return NULL;
	}
	lockpool->lockList = NULL;
	lockpool->size = 0;
	lockpool->index = 0;

	return lockpool;
}

int LockPool_Init(LockPool* lockpool,unsigned int size)
{
	SYSTEM_INFO sysInfo;
	unsigned int i = 0;

	lockpool->size = size;
	if(lockpool->size == 0)
	{
		GetSystemInfo(&sysInfo);
		lockpool->size = sysInfo.dwNumberOfProcessors;
	}
	if(lockpool->size <= 0){
		lockpool->size = 2;
	}

	lockpool->lockList = xMalloc(lockpool->size * sizeof(Lock*));
	for(i = 0; i < lockpool->size; ++i){
		lockpool->lockList[i] = Lock_New();
	}

	lockpool->index = 0;
	return TRUE;
}

int LockPool_Destroy(LockPool* lockpool)
{
	unsigned int i = 0;
	if(lockpool->lockList && lockpool->size > 0)
	{
		for(i = 0; i < lockpool->size; ++i)
		{
			Lock_Destory(lockpool->lockList[i], TRUE);
		}
		xFree(lockpool->lockList);
	}
	lockpool->lockList = NULL;
	lockpool->size = 0;
	lockpool->index = 0;

	return TRUE;
}



Lock* LockPool_Allocate(LockPool* lockpool)
{
	return lockpool->lockList[(lockpool->index++) % lockpool->size];
}



void LockPool_Recycle(Lock* lockPtr)
{

}