#ifndef THREAD_H
#define THREAD_H

//////////////////////////////////////////////////////////////////////////
//written by hetao.su
//suhetao@gmail.com
//////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <limits.h>
#include <process.h>

#define SPIN_COUNT 0
#define THREAD_MUTEX_INITIALIZER {0}
#define THREAD_COND_INITIALIZER {0}

//simulate the VISTA CONDITION_VARIABLE
//use for pointing to a THREAD_CONDITION struct
typedef struct _CONDITION_VARIABLE
{
	VOID *pPTR;
} CONDITION_VAR;

typedef VOID* (*FnPtr)(VOID*);

typedef struct THREAD_HANDLE
{
    VOID *hHandle;
	DWORD dwThreadID;
    FnPtr pFn;
    VOID *pArg;
    VOID **ppRet;
    VOID *pRet;
} ThreadHandle;

typedef struct THREAD_CONDITION
{
	CRITICAL_SECTION mutexBroadcast;
	CRITICAL_SECTION mutexWaiterCount;
	INT iWaiterCount;
	HANDLE hSemaphore;
	HANDLE hWaitersDone;
	volatile INT iIsBroadcast;
} ThreadCondition;

typedef struct THREAD_CONTROL
{
	CRITICAL_SECTION globalMutex;
	VOID (WINAPI *CondInitPtr)(CONDITION_VAR *);
	VOID (WINAPI *CondBroadcastPtr)(CONDITION_VAR *);
	VOID (WINAPI *CondSignalPtr)(CONDITION_VAR *);
	BOOL (WINAPI *CondWaitPtr)(CONDITION_VAR *, CRITICAL_SECTION *, DWORD);
} Thread_Control;

#ifdef __cplusplus
extern "C" {
#endif

INT Thread_Create(ThreadHandle *pThreadInfo, INT* pAttr, VOID *(*StartRoutinePtr)(VOID*), VOID *pArg);
INT Thread_Join(ThreadHandle threadInfo, VOID **ppValuePtr);

INT Thread_MutexInit(CRITICAL_SECTION *pMutex,INT *pAttr);
INT Thread_MutexDestroy(CRITICAL_SECTION *pMutex);
INT Thread_MutexLock(CRITICAL_SECTION *pMutex);
INT Thread_MutexUnlock(CRITICAL_SECTION *pMutex);

INT Thread_CondInit(CONDITION_VAR *pCond, INT *pAttr);
INT Thread_CondDestroy(CONDITION_VAR *pCond);
INT Thread_CondBroadcast(CONDITION_VAR *pCond);
INT Thread_CondWait(CONDITION_VAR *pCond, CRITICAL_SECTION *pMutex);
INT Thread_CondSignal(CONDITION_VAR *pCond);

INT  Thread_Init(VOID);
VOID Thread_Destroy(VOID);

INT MemCmp(VOID *pSrc,VOID *pDest,LONG lNum);
INT GetNumberProcessors(VOID);

DWORD WINAPI ThreadWorker(LPVOID lpParam);

#ifdef __cplusplus
}
#endif

#endif