#include "Thread.h"

//ugly compatible with xp
//use these global variable for thread condition wait
static CRITICAL_SECTION g_Mutex_Init = THREAD_MUTEX_INITIALIZER;
static Thread_Control g_Thread_Control = THREAD_COND_INITIALIZER;

INT Thread_Create(ThreadHandle *pThreadHandle,INT* pAttr,VOID *(*StartRoutinePtr)(VOID*), VOID *pArg)
{
    pThreadHandle->pFn   = StartRoutinePtr;
    pThreadHandle->pArg    = pArg;
    pThreadHandle->ppRet  = &pThreadHandle->pRet;
    pThreadHandle->pRet    = NULL;
    pThreadHandle->hHandle = (VOID*)CreateThread((LPSECURITY_ATTRIBUTES)pAttr, 0, ThreadWorker, pThreadHandle, 0, &pThreadHandle->dwThreadID);
    return !pThreadHandle->hHandle;
}

INT Thread_Join(ThreadHandle threadHandle, VOID **ppValuePtr)
{
    DWORD dRet = WaitForSingleObject(threadHandle.hHandle, INFINITE);
    if(dRet != WAIT_OBJECT_0)
        return -1;
    if(ppValuePtr)
        *ppValuePtr = *threadHandle.ppRet;
    CloseHandle(threadHandle.hHandle);
    return 0;
}

INT Thread_MutexInit(CRITICAL_SECTION *pMutex, INT * pAttr)
{
    return !InitializeCriticalSectionAndSpinCount(pMutex, SPIN_COUNT);
}

INT Thread_MutexDestroy(CRITICAL_SECTION *pMutex)
{
    DeleteCriticalSection(pMutex);
    return 0;
}

INT Thread_MutexLock(CRITICAL_SECTION *pMutex)
{
	//ugly compatible between xp and vista later
	//in xp or before system version,we can't use condition variable
	//so we just use a global mutex to implement condition wait
    if(!MemCmp(pMutex, &g_Mutex_Init, sizeof(CRITICAL_SECTION)))
        *pMutex = g_Thread_Control.globalMutex;
    EnterCriticalSection(pMutex);
    return 0;
}

INT Thread_MutexUnlock(CRITICAL_SECTION *pMutex)
{
    LeaveCriticalSection(pMutex);
    return 0;
}

INT Thread_CondInit(CONDITION_VAR *pCond, INT *	pAttr)
{
	ThreadCondition *pThreadCondition;
    if(g_Thread_Control.CondInitPtr)
    {
        g_Thread_Control.CondInitPtr(pCond);
        return 0;
    }

    pThreadCondition = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,sizeof(ThreadCondition));
    if(!pThreadCondition)
        return -1;
    pCond->pPTR = pThreadCondition;
    pThreadCondition->hSemaphore = CreateSemaphore(NULL, 0, INT_MAX, NULL);
    if(!pThreadCondition->hSemaphore)
        return -1;

    if(Thread_MutexInit(&pThreadCondition->mutexWaiterCount,NULL))
        return -1;
    if(Thread_MutexInit(&pThreadCondition->mutexBroadcast,NULL))
        return -1;

    pThreadCondition->hWaitersDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!pThreadCondition->hWaitersDone)
        return -1;

    return 0;
}

INT ThreadCondDestroy(CONDITION_VAR *pCond)
{
	ThreadCondition *pThreadCondition = pCond->pPTR;
    if(g_Thread_Control.CondInitPtr)
        return 0;

    CloseHandle(pThreadCondition->hSemaphore);
    CloseHandle(pThreadCondition->hWaitersDone);
    Thread_MutexDestroy(&pThreadCondition->mutexBroadcast);
    Thread_MutexDestroy(&pThreadCondition->mutexWaiterCount);
	HeapFree(GetProcessHeap(), 0, pThreadCondition);

    return 0;
}

INT Thread_CondBroadcast(CONDITION_VAR *pCond)
{
	ThreadCondition *pThreadCondition = pCond->pPTR;
	INT iHaveWaiter = 0;

    if(g_Thread_Control.CondBroadcastPtr)
    {
        g_Thread_Control.CondBroadcastPtr(pCond);
        return 0;
    }

    Thread_MutexLock(&pThreadCondition->mutexBroadcast);
    Thread_MutexLock(&pThreadCondition->mutexWaiterCount);
 
    if(pThreadCondition->iWaiterCount)
    {
        pThreadCondition->iIsBroadcast = 1;
        iHaveWaiter = 1;
    }

    if(iHaveWaiter)
    {
        ReleaseSemaphore(pThreadCondition->hSemaphore, pThreadCondition->iWaiterCount, NULL);
        Thread_MutexUnlock(&pThreadCondition->mutexWaiterCount);
        WaitForSingleObject(pThreadCondition->hWaitersDone, INFINITE);
        pThreadCondition->iIsBroadcast = 0;
    }
    else
        Thread_MutexUnlock(&pThreadCondition->mutexWaiterCount);
    return Thread_MutexUnlock(&pThreadCondition->mutexBroadcast);
}

INT Thread_CondWait(CONDITION_VAR *pCond, CRITICAL_SECTION *pMutex)
{
	ThreadCondition *pThreadCondition = pCond->pPTR;
	INT iLastWaiter;

	if(g_Thread_Control.CondWaitPtr)
		return !g_Thread_Control.CondWaitPtr(pCond, pMutex, INFINITE);

	Thread_MutexLock(&pThreadCondition->mutexBroadcast);
	Thread_MutexLock(&pThreadCondition->mutexWaiterCount);
	pThreadCondition->iWaiterCount++;
	Thread_MutexUnlock(&pThreadCondition->mutexWaiterCount);
	Thread_MutexUnlock(&pThreadCondition->mutexBroadcast);

	Thread_MutexUnlock(pMutex);
	WaitForSingleObject(pThreadCondition->hSemaphore, INFINITE);

	Thread_MutexLock(&pThreadCondition->mutexWaiterCount);
	pThreadCondition->iWaiterCount--;
	iLastWaiter = !pThreadCondition->iWaiterCount || !pThreadCondition->iIsBroadcast;
	Thread_MutexUnlock(&pThreadCondition->mutexWaiterCount);

	if(iLastWaiter)
		SetEvent(pThreadCondition->hWaitersDone);

	return Thread_MutexLock(pMutex);
}

INT Thread_CondSignal(CONDITION_VAR *pCond)
{
	INT iHaveWaiter;
	ThreadCondition *pThreadCondition = pCond->pPTR;

    if(g_Thread_Control.CondSignalPtr)
    {
        g_Thread_Control.CondSignalPtr(pCond);
        return 0;
    }

    Thread_MutexLock(&pThreadCondition->mutexBroadcast);
    Thread_MutexLock(&pThreadCondition->mutexWaiterCount);
    iHaveWaiter = pThreadCondition->iWaiterCount;
    Thread_MutexUnlock(&pThreadCondition->mutexWaiterCount);

    if(iHaveWaiter)
    {
        ReleaseSemaphore(pThreadCondition->hSemaphore, 1, NULL);
        WaitForSingleObject(pThreadCondition->hWaitersDone, INFINITE);
    }

    return Thread_MutexUnlock(&pThreadCondition->mutexBroadcast);
}

INT Thread_Init(VOID)
{
    HANDLE kernel_dll = GetModuleHandle(TEXT("kernel32.dll"));

	g_Thread_Control.CondInitPtr = (VOID*)GetProcAddress(kernel_dll, "InitializeConditionVariable");
    if(g_Thread_Control.CondInitPtr)
    {
        g_Thread_Control.CondBroadcastPtr = (VOID*)GetProcAddress(kernel_dll, "WakeAllConditionVariable");
        g_Thread_Control.CondSignalPtr = (VOID*)GetProcAddress(kernel_dll, "WakeConditionVariable");
        g_Thread_Control.CondWaitPtr = (VOID*)GetProcAddress(kernel_dll, "SleepConditionVariableCS");
    }
    return Thread_MutexInit(&g_Thread_Control.globalMutex,NULL);
}

VOID Thread_Destroy(VOID)
{
    Thread_MutexDestroy(&g_Thread_Control.globalMutex);
	ZeroMemory(&g_Thread_Control,sizeof(Thread_Control));
}

DWORD WINAPI ThreadWorker(VOID *pArg)
{
	ThreadHandle *pThreadHandle = pArg;
	*pThreadHandle->ppRet = pThreadHandle->pFn(pThreadHandle->pArg);
	return 0;
}

INT GetNumberProcessors(VOID)
{
    DWORD_PTR systemCpus, processCpus = 0;
    INT iCpus = 0;
	DWORD_PTR dBit;

    if(!processCpus)
        GetProcessAffinityMask(GetCurrentProcess(), &processCpus, &systemCpus);
    for(dBit = 1; dBit; dBit <<= 1)
        iCpus += !!(processCpus & dBit);

    return iCpus ? iCpus : 1;
}

INT MemCmp(VOID *pSrc,VOID *pDest,LONG lNum)
{
	UINT8 *pSrcpPtr,*pDestPtr;
	if(pSrc == NULL||pDest == NULL)
		return -1;
	pSrcpPtr = pSrc;
	pDestPtr = pDest;
	for(;0 < lNum;++pSrcpPtr,++pDestPtr,--lNum)
		if(*pSrcpPtr != *pDestPtr)
			return(( *pSrcpPtr < *pDestPtr) ? -1 : +1);
	return 0;
}