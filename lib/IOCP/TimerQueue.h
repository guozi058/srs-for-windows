#ifndef _TIMERQUEUE_H_
#define _TIMERQUEUE_H_

#include "List.h"
#include "Vector.h"
#include "Lock.h"

#define DEFAULT_TIMERPOOL_SIZE 128

typedef enum{
	OP_ADD = 1,
	OP_CHANGE = 2,
	OP_REMOVE = 3
}Operation_Type;

typedef enum{
	WK_UNDIFINE = 0,
	WK_TIMEOUT = 1,
	WK_RESET = 2
}WakeUp_Type;


#define TIMER_STATE_NORMAL 0
#define TIMER_STATE_PROCESSING 1
#define TIMER_STATE_EXPIRED 2
#define TQ_SUCCESS 0
#define TQ_ERROR 1

typedef void* Timer;
typedef void (__stdcall *TimerCallBack)(void*);

typedef struct _TIMERDES_T
{
	TimerCallBack callback;
	void* param;
	HANDLE event;
	__int64 expireCounters;
	int state;
	int waitting;
	int autoDelete;
}TimerDesc;

typedef struct _OPDESC_T
{
	TimerDesc* timerDescPtr;
	int opType;
}OpDesc;

typedef struct _TIMERQUEUE_T TimerQueue;
typedef DWORD (__stdcall *Worker)(void* param);
typedef void (*Wakeup)(TimerQueue*);
typedef int (*Proc)(TimerQueue* tq, HANDLE event);

typedef struct _TIMERQUEUE_T
{	
	HANDLE waitableTimer;
	HANDLE timerThread;
	int wakeupType;
	int stop;

	LIST* timerList;
	TimerDesc* curTimer;
	CRITICAL_SECTION cs;
	__int64 frequency;
	Vector* opVector;

	unsigned int poolSize;
	unsigned int poolPos;
	TimerDesc** timerPool;

	Worker worker;
	Wakeup wakeup;
	Proc proc;
}TimerQueue;

#ifdef __cplusplus
extern "C"
{
#endif

__int64 now(__int64 frequency);
__int64 getCounters(__int64 frequency, __int64 ms);
__int64 getMs(__int64 frequency, __int64 counter);

DWORD __stdcall TimerQueue_Worker(void* param);

TimerDesc* TimerQueue_AllocTimer(TimerQueue*);
int TimerQueue_FreeTimer(TimerQueue*,TimerDesc*);
int TimerQueue_Proc(TimerQueue* tq, HANDLE event);
int TimerQueue_SetNextTimer(TimerQueue* tq);
TimerDesc* TimerQueue_GetFirstTimer(TimerQueue*);
int TimerQueue_IsFirstTimer(TimerQueue* tq, TimerDesc*);
int TimerQueue_IsValidTimer(TimerQueue* tq, TimerDesc*);
int TimerQueue_Inqueue(TimerQueue* tq, TimerDesc*);
void TimerQueue_Wakeup(TimerQueue* tq);
void TimerQueue_OpAcquire(TimerQueue* tq, TimerDesc* timerPtr, int oppType);
void TimerQueue_OpExecute(TimerQueue* tq);
TimerQueue* TimerQueue_New(int poolSize);
int TimerQueue_Destory(TimerQueue* tq);
int TimerQueue_Init(TimerQueue* tq);
Timer TimerQueue_CreateTimer(TimerQueue* tq, DWORD timeout, TimerCallBack callbackFunc, void* param);
int TimerQueue_ChangeTimer(TimerQueue* tq, Timer timerPtr, DWORD timeout);
int TimerQueue_DeleteTimer(TimerQueue* tq, Timer timerPtr, int wait);

#ifdef __cplusplus
}
#endif

#endif