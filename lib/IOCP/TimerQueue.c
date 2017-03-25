#include "TimerQueue.h"
#include "Util.h"

__int64 now(__int64 frequency)
{
	LARGE_INTEGER counter;
	if(frequency == 0)
	{
		return GetTickCount();
	}
	else
	{
		if(!QueryPerformanceCounter(&counter)) return GetTickCount();
		return counter.QuadPart;
	}
}

__int64 getCounters(__int64 frequency, __int64 ms)
{
	if( 0 == frequency )
	{
		return ms;
	}
	else
	{
		return ms * frequency / 1000;
	}
}

__int64 getMs(__int64 frequency, __int64 counter)
{
	if( 0 == frequency ){
		return counter;
	}
	else
	{
		return (__int64)((counter * 1.0 / frequency) * 1000);
	}
}

TimerQueue *TimerQueue_New(int poolSize)
{
	LARGE_INTEGER freq;
	TimerQueue* tq = NULL;

	tq = xMalloc(sizeof(TimerQueue));
	if (NULL == tq){
		return NULL;
	}

	tq->waitableTimer = NULL;
	tq->timerThread = NULL;
 	
	if(!QueryPerformanceFrequency(&freq)){
		tq->frequency = 0;
	}
	else{
		tq->frequency = freq.QuadPart;
	}
	InitializeCriticalSection(&tq->cs);
	tq->timerList = NULL;
	if (poolSize <= 0){
		tq->poolSize = DEFAULT_TIMERPOOL_SIZE;
	}
	else{
		tq->poolSize = poolSize;
	}
	tq->poolPos = 0;
	tq->timerPool = NULL;
	tq->opVector = NULL;
	tq->wakeupType = WK_UNDIFINE;
	tq->curTimer = NULL;
	tq->stop = FALSE;

	return tq;
}

int TimerQueue_Init(TimerQueue* tq)
{
	if (NULL == tq){
		return TQ_ERROR;
	}
	if( NULL == (tq->waitableTimer = CreateWaitableTimer(NULL, FALSE, NULL)))
	{
		return TQ_ERROR;
	}

	tq->timerList = List_New(sizeof(TimerDesc));
	if(tq->poolSize > 0){	
		tq->timerPool = xMalloc(tq->poolSize * sizeof(TimerDesc*));
	}
	tq->opVector = Vector_New(sizeof(OpDesc), 0);
	if( 0 == (tq->timerThread = CreateThread(NULL, 0, tq->worker, tq, 0, NULL))){
		return TQ_ERROR;
	}
	tq->stop = FALSE;
	return TQ_SUCCESS;
}

int TimerQueue_Destory(TimerQueue* tq)
{
	unsigned int i = 0;
	LIST_NODE* timer = NULL;

	if(tq->timerThread)
	{
		EnterCriticalSection(&tq->cs);
		tq->stop = TRUE;
		tq->wakeup(tq);
		LeaveCriticalSection(&tq->cs);

		WaitForSingleObject(tq->timerThread, INFINITE);
		CloseHandle(tq->timerThread);
		tq->timerThread = NULL;
	}
	if(tq->waitableTimer)
	{
		CloseHandle(tq->waitableTimer);
		tq->waitableTimer = NULL;
	}
	if(tq->timerList)
	{
		for (timer = List_Front(tq->timerList); NULL != timer; timer = timer->next){
			TimerQueue_FreeTimer(tq, timer->element);
		}
		if(List_Size(tq->timerList) > 0){
			List_Clear(tq->timerList);
		}
		xFree(tq->timerList->head);
		xFree(tq->timerList);
		tq->timerList = NULL;
	}

	if(tq->timerPool)
	{
		for(i = 0; i < tq->poolPos; ++i)
		{
			xFree(tq->timerPool[i]);
		}
		xFree(tq->timerPool);
		tq->timerPool = NULL;
		tq->poolPos = 0;
	}

	if(tq->opVector)
	{
		Vector_Free(&tq->opVector);
		tq->opVector = NULL;
	}

	DeleteCriticalSection(&tq->cs);

	tq->stop = FALSE;
	tq->curTimer = NULL;
	tq->wakeupType = WK_UNDIFINE;
	return TQ_SUCCESS;
}

TimerDesc* TimerQueue_AllocTimer(TimerQueue* tq)
{
	TimerDesc* timerPtr = NULL;
	if(tq->timerPool && tq->poolPos > 0)
	{
		timerPtr = tq->timerPool[--tq->poolPos];
	}
	else
	{
		timerPtr = xMalloc(sizeof(TimerDesc));
	}
	xMemSet(timerPtr, 0, sizeof(TimerDesc));
	return timerPtr;
}

int TimerQueue_FreeTimer(TimerQueue* tq, TimerDesc* timerPtr)
{
	if(tq->timerPool && tq->poolPos < tq->poolSize){
		tq->timerPool[tq->poolPos++] = timerPtr;
	}
	else{
		xFree(timerPtr);
	}
	return 0;
}

int TimerQueue_IsValidTimer(TimerQueue* tq, TimerDesc* timerPtr)
{
	return timerPtr->state == TIMER_STATE_NORMAL;
}

TimerDesc* TimerQueue_GetFirstTimer(TimerQueue* tq)
{
	LIST_NODE* node = NULL;
	unsigned int size = 0;
	unsigned int i = 0;

	size = List_Size(tq->timerList);

	for (i = 0; i < size; i++){
		node = List_At(tq->timerList, i);
		if(TimerQueue_IsValidTimer(tq, node->element))
		{
			return node->element;
		}

	}
	return NULL;
}

int TimerQueue_IsFirstTimer(TimerQueue* tq, TimerDesc* timerPtr)
{
	return TimerQueue_GetFirstTimer(tq) == timerPtr;
}

int TimerQueue_SetNextTimer(TimerQueue* tq)
{
	LARGE_INTEGER dueTime;
	__int64 curCounters = 0;
	__int64 ms = 0;

	tq->wakeupType = WK_TIMEOUT;
	tq->curTimer = TimerQueue_GetFirstTimer(tq);
	if(NULL == tq->curTimer)
	{
		if(!CancelWaitableTimer(tq->waitableTimer)){
		}
	}
	else
	{
		
		curCounters = now(tq->frequency);
		if(curCounters >= tq->curTimer->expireCounters)
		{
			dueTime.QuadPart = 0;
		}
		else
		{
			ms = getMs(tq->frequency, tq->curTimer->expireCounters - curCounters);
			dueTime.QuadPart = ms * 1000 * 10;
			dueTime.QuadPart = ~dueTime.QuadPart + 1;
		}
		
		if(!SetWaitableTimer(tq->waitableTimer, &dueTime, 0, NULL, NULL, FALSE)){
		}
	}
	return 0;
}

void TimerQueue_OpExecute(TimerQueue* tq)
{
	unsigned int size = 0;
	unsigned int i = 0;
	OpDesc* opdesc = NULL;

	if(!Vector_Empty(tq->opVector))
	{
		size = Vector_Size(tq->opVector);
		for(i = 0; i < size; i++)
		{
			opdesc = Vector_At(tq->opVector, i);
			if(OP_ADD == opdesc->opType)
			{
				//tq->inqueue(tq, opdesc->timerDescPtr);
				TimerQueue_Inqueue(tq, opdesc->timerDescPtr);
			}
			else if(OP_CHANGE == opdesc->opType)
			{
				List_Remove(tq->timerList, opdesc->timerDescPtr);
				//tq->inqueue(tq, opdesc->timerDescPtr);
				TimerQueue_Inqueue(tq, opdesc->timerDescPtr);
			}
			else if(OP_REMOVE == opdesc->opType)
			{
				List_Remove(tq->timerList, opdesc->timerDescPtr);
				//tq->freeTimer(tq, opdesc->timerDescPtr);
				TimerQueue_FreeTimer(tq, opdesc->timerDescPtr);
			}
			else{
			}
		}
		Vector_Clear(tq->opVector);
	}
}

int TimerQueue_Proc(TimerQueue* tq, HANDLE event)
{
	TimerDesc* timerDescPtr = NULL;

	EnterCriticalSection(&tq->cs);
	if(tq->stop)
	{
		LeaveCriticalSection(&tq->cs);
		return FALSE;
	}
	if(WK_RESET == tq->wakeupType)
	{
		TimerQueue_OpExecute(tq);
	}
	else
	{
		timerDescPtr = tq->curTimer;
		timerDescPtr->state = TIMER_STATE_PROCESSING;
		timerDescPtr->event = event;
		LeaveCriticalSection(&tq->cs);

		if(timerDescPtr->callback){
			timerDescPtr->callback(timerDescPtr->param);
		}
	
		EnterCriticalSection(&tq->cs);
		timerDescPtr->state = TIMER_STATE_EXPIRED;
		if(timerDescPtr->waitting)
		{
			SetEvent(timerDescPtr->event);
		}
		if(timerDescPtr->autoDelete)
		{
			//_timerList->remove(timerDescPtr);
			TimerQueue_FreeTimer(tq, timerDescPtr);
		}
		TimerQueue_OpExecute(tq);
	}
	TimerQueue_SetNextTimer(tq);

	LeaveCriticalSection(&tq->cs);
	return  TRUE;
}

DWORD __stdcall TimerQueue_Worker(void* param)
{
	TimerQueue* instPtr = (TimerQueue*)(param);
	HANDLE completeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	int ret = 0;

	if(!completeEvent)
	{
		return 1;
	}

	while(TRUE)
	{
		if( WAIT_OBJECT_0 == WaitForSingleObject(instPtr->waitableTimer, INFINITE) )
		{
			//if(!instPtr->proc(instPtr, completeEvent))
			if(!TimerQueue_Proc(instPtr, completeEvent))
			{
				break;
			}
		}
		else
		{
			ret = 2;
			break;
		}
	}
	CloseHandle(completeEvent);
	return ret;
}

int TimerQueue_Inqueue(TimerQueue* tq, TimerDesc* newTimerPtr)
{
	int inserted = FALSE;
	TimerDesc* timerPtr = NULL;
	LIST_NODE* node = NULL;
	unsigned int size = 0;

	if (!List_Empty(tq->timerList))
	{
		node = List_Back(tq->timerList);
		size = List_Size(tq->timerList);
		do
		{
			--size;
			timerPtr = node->element;
			//if(tq->isValidTimer(tq, timerPtr) && newTimerPtr->expireCounters >= timerPtr->expireCounters)
			if(TimerQueue_IsValidTimer(tq, timerPtr) && newTimerPtr->expireCounters >= timerPtr->expireCounters)
			{
				++size;
				List_Insert(tq->timerList, newTimerPtr, size);
				inserted = TRUE;
				break;
			}
			node = node->prev;
		}while(node != NULL);
	}

	if(!inserted)
	{
		List_Push_Front(tq->timerList, newTimerPtr);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

Timer TimerQueue_CreateTimer(TimerQueue* tq, DWORD timeout, TimerCallBack callbackFunc, void* param)
{
	TimerDesc* newTimerPtr = NULL;
	EnterCriticalSection(&tq->cs);
	newTimerPtr = TimerQueue_AllocTimer(tq);
	newTimerPtr->expireCounters = now(tq->frequency) + getCounters(tq->frequency, timeout);
	newTimerPtr->state = TIMER_STATE_NORMAL;
	newTimerPtr->callback = callbackFunc;
	newTimerPtr->param = param;
	newTimerPtr->waitting = FALSE;

	//tq->opAcquire(tq, newTimerPtr, OP_ADD);
	TimerQueue_OpAcquire(tq, newTimerPtr, OP_ADD);
	LeaveCriticalSection(&tq->cs);

	return newTimerPtr;
}

int TimerQueue_ChangeTimer(TimerQueue* tq, Timer timerPtr, DWORD timeout)
{
	TimerDesc* timerDescPtr = (TimerDesc*)(timerPtr);
	EnterCriticalSection(&tq->cs);
	timerDescPtr->expireCounters = now(tq->frequency) +getCounters(tq->frequency, timeout);
	timerDescPtr->state = TIMER_STATE_NORMAL;
	timerDescPtr->waitting = FALSE;

	//tq->opAcquire(tq, timerDescPtr, OP_CHANGE);
	TimerQueue_OpAcquire(tq, timerDescPtr, OP_CHANGE);
	LeaveCriticalSection(&tq->cs);
	return TQ_SUCCESS;
}

int TimerQueue_DeleteTimer(TimerQueue* tq, Timer timerPtr, int wait)
{
	int ret = TQ_SUCCESS;
	TimerDesc* timerDescPtr = (TimerDesc*)(timerPtr);
	HANDLE event = NULL;

	EnterCriticalSection(&tq->cs);
	if(timerDescPtr->state != TIMER_STATE_PROCESSING)
	{
		TimerQueue_OpAcquire(tq, timerDescPtr, OP_REMOVE);
	}
	else
	{
		timerDescPtr->waitting = wait;
		timerDescPtr->autoDelete = TRUE;

		if(wait)
		{
			event = timerDescPtr->event;
		}
	}
	LeaveCriticalSection(&tq->cs);
	if(event != NULL)
	{
		WaitForSingleObject(event, INFINITE);
	}
	return ret;
}

void TimerQueue_OpAcquire(TimerQueue* tq, TimerDesc* timerPtr, int oppType)
{
	int wake = FALSE;

	OpDesc* opdesc = NULL;
	opdesc = xMalloc(sizeof(OpDesc));
	if (NULL == opdesc){
		return;
	}
	opdesc->timerDescPtr = timerPtr;
	opdesc->opType = oppType;
	Vector_Push_Back(tq->opVector,opdesc);
	xFree(opdesc);

	if(oppType == OP_ADD){
		wake = (tq->curTimer == NULL || timerPtr->expireCounters < tq->curTimer->expireCounters);
	}
	else if(oppType == OP_CHANGE){
		wake = (timerPtr == tq->curTimer || timerPtr->expireCounters < tq->curTimer->expireCounters);
	}
	else{
		wake = (tq->curTimer == NULL || timerPtr == tq->curTimer);
	}
	
	if(wake)
	{
		//tq->wakeup(tq);
		TimerQueue_Wakeup(tq);
	}
}

void TimerQueue_Wakeup(TimerQueue* tq)
{
	LARGE_INTEGER dueTime;

	if(tq->wakeupType != WK_RESET)
	{
		tq->wakeupType = WK_RESET;
		dueTime.QuadPart = 0;

		if(!SetWaitableTimer(tq->waitableTimer, &dueTime, 0, NULL, NULL, FALSE))
		{
		}
	}
	else
	{
	}
}