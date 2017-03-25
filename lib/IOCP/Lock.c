#include "Lock.h"

Lock* Lock_New()
{
	Lock* lock = NULL;
	lock = xMalloc(sizeof(Lock));
	if (NULL == lock){
		return NULL;
	}
	lock->readLockCount = 0;
	lock->state = LOCK_IDLE;
	lock->readWaitingCount = 0;
	lock->writeWaitingCount = 0;
	InitializeCriticalSection(&lock->cs);
	lock->event = CreateEvent(NULL, TRUE, FALSE, NULL);

	return lock;
}

void Lock_Init(Lock* lock)
{
	lock->readLockCount = 0;
	lock->state = LOCK_IDLE;
	lock->readWaitingCount = 0;
	lock->writeWaitingCount = 0;
	InitializeCriticalSection(&lock->cs);
	lock->event = CreateEvent(NULL, TRUE, FALSE, NULL);
}

void Lock_Destory(Lock* lock, unsigned int flag)
{
	if (NULL == lock){
		return;
	}
	DeleteCriticalSection(&lock->cs);
	CloseHandle(lock->event);
	if (flag){
		xFree(lock);
	}
}

void Lock_Read(Lock* lock)
{
	int isWaitReturn = FALSE;
	while(TRUE)
	{
		EnterCriticalSection(&lock->cs);
		if(isWaitReturn)
		{
			--lock->readWaitingCount;
		}

		if(lock->state == LOCK_IDLE)
		{
			lock->state = LOCK_R;
			lock->readLockCount++;
			LeaveCriticalSection(&lock->cs);
			break;
		}
		else if( lock->state == LOCK_R)
		{
			if(lock->writeWaitingCount > 0)
			{
				++lock->readWaitingCount;
				ResetEvent(lock->event);
				LeaveCriticalSection(&lock->cs);
				WaitForSingleObject(lock->event, INFINITE);
				isWaitReturn = TRUE;
			}
			else
			{
				++lock->readLockCount;
				LeaveCriticalSection(&lock->cs);
				break;
			}
		}
		else if(lock->state == LOCK_W)
		{
			++lock->readWaitingCount;
			ResetEvent(lock->event);
			LeaveCriticalSection(&lock->cs);
			WaitForSingleObject(lock->event, INFINITE);
			isWaitReturn = TRUE;
		}
		else
		{
			break;
		}
	}
}

void Lock_Write(Lock* lock)
{
	int isWaitReturn = FALSE;
	while(TRUE)
	{
		EnterCriticalSection(&lock->cs);

		if(isWaitReturn) --lock->writeWaitingCount;

		if(lock->state == LOCK_IDLE)
		{
			lock->state = LOCK_W;
			LeaveCriticalSection(&lock->cs);
			break;
		}
		else
		{
			++lock->writeWaitingCount;
			ResetEvent(lock->event);
			LeaveCriticalSection(&lock->cs);
			WaitForSingleObject(lock->event, INFINITE);

			isWaitReturn = TRUE;
		}
	}
}

void Lock_Unlock(Lock* lock)
{
	EnterCriticalSection(&lock->cs);
	if(lock->readLockCount > 0)
	{
		--lock->readLockCount;

		if( 0 == lock->readLockCount)
		{
			lock->state = LOCK_IDLE;
			if( lock->writeWaitingCount > 0 || lock->readWaitingCount > 0 ){
				SetEvent(lock->event);
			}
			else{
			}
		}
		else{
		}
	}
	else
	{
		lock->state = LOCK_IDLE;
		if( lock->writeWaitingCount > 0 || lock->readWaitingCount > 0 ){
			SetEvent(lock->event);
		}
		else{
		}
	}
	LeaveCriticalSection(&lock->cs);
}