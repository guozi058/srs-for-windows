#include "IOCP.h"

int InitWinsock(WORD nMainVer, WORD nSubVer)
{
	WORD wVer;
	WSADATA ws;
	wVer = MAKEWORD(nMainVer, nSubVer);
	return WSAStartup(wVer, &ws) == 0;
}

int CleanWinsock()
{
	return WSACleanup() == 0;
}

IOCP* IOCP_New(void)
{
	IOCP* iocp = NULL;
	LARGE_INTEGER freq;

	iocp = xMalloc(sizeof(IOCP));
	if(NULL == iocp){
		return NULL;
	}
	iocp->iocpHandle = NULL;
	iocp->threads = 0;
	iocp->hThreads = NULL;
	iocp->lastErrorCode = 0;
	iocp->inited = FALSE;
	if(!QueryPerformanceFrequency(&freq)){
		iocp->frequency = 0;
	}
	else{
		iocp->frequency = freq.QuadPart;
	}
	return iocp;
}

int IOCP_Init(IOCP* iocp, int threads)
{
	SYSTEM_INFO sysInfo;
	int i = 0;

	if(threads <= 0)
	{
		GetSystemInfo(&sysInfo);
		threads = sysInfo.dwNumberOfProcessors;
	}
	if(threads <= 0 || threads > 64)
		threads = 5; 

	if( NULL == (iocp->iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)NULL, threads)) )
	{
		iocp->lastErrorCode = GetLastError();
		//iocp->destory(iocp);
		IOCP_Destroy(iocp);
		return IOCP_UNDEFINED;
	}

	TimerQueue_Init(&iocp->timerQueue);
	LockPool_Init(&iocp->lockPool, 0);

	iocp->threads = threads;
	if(iocp->hThreads)
		xFree(iocp->hThreads);
	iocp->hThreads = xMalloc(sizeof(HANDLE)* threads);
	xMemSet(iocp->hThreads, 0, sizeof(HANDLE) * threads);

	for(i = 0; i < iocp->threads; ++i)
	{
		//if(0 == (iocp->hThreads[i] = _beginthreadex(NULL, 0, IOCP_Service, iocp, 0, NULL)))
		if(NULL == (iocp->hThreads[i] = CreateThread(NULL, 0, IOCP_Service, iocp, 0, NULL)))
		{
			//iocp->lastErrorCode = errno;
			iocp->lastErrorCode = GetLastError();
			//iocp->destroy(iocp);
			IOCP_Destroy(iocp);
			return IOCP_UNDEFINED;
		}
	}
	iocp->contextMap = Map_New();

	InitializeCriticalSection(&iocp->cs);
	iocp->inited = TRUE;
	return IOCP_SUCESS;
}

static void IOCP_ContextHandler(Rb_Node* node)
{
	IOCP_CONTEXT *context = (IOCP_CONTEXT *)node->value;

	if(!(context->status & IOCP_HANDLE_CLOSED))
	{
		//context->instPtr->closeHandle(context->instPtr, context);
		IOCP_CloseHandle(context->instPtr, context);
		context->status |= IOCP_HANDLE_CLOSED;
	}
	//iocp->freeContext(context->instPtr, context);
	IOCP_FreeContext(context->instPtr, context);
}

int IOCP_Destroy(IOCP* iocp)
{
	HANDLE curThread = NULL;
	int i = 0;
	if(iocp->hThreads)
	{
		curThread = GetCurrentThread();
		for(i = 0; i < iocp->threads; ++i)
		{
			if(curThread == iocp->hThreads[i])
				return IOCP_DESTROYFAILED;
		}
	}

	iocp->inited = FALSE;
	if(iocp->hThreads)
	{
		if(iocp->iocpHandle)
		{
			for(i = 0; i < iocp->threads; ++i){
				PostQueuedCompletionStatus(iocp->iocpHandle, 0, (ULONG_PTR)NULL, NULL);
			}
		}

		for(i = 0; i < iocp->threads; ++i)
		{
			if( iocp->hThreads[i] ) 
			{
				WaitForSingleObject(iocp->hThreads[i], INFINITE);
				CloseHandle(iocp->hThreads[i]);
			}
		}
		xFree(iocp->hThreads);
	}
	iocp->hThreads = NULL;
	iocp->threads = 0;

	if(iocp->iocpHandle)
	{
		if(!CloseHandle(iocp->iocpHandle)){
		}
		iocp->iocpHandle = NULL;
	}
	TimerQueue_Destory(&iocp->timerQueue);

	Map_Foreach(iocp->contextMap, IOCP_ContextHandler);
	Map_Delete(iocp->contextMap);

	LockPool_Destroy(&iocp->lockPool);
	DeleteCriticalSection(&iocp->cs);

	return IOCP_SUCESS;
}

IOCP_CONTEXT* IOCP_AllocContext(IOCP* iocp)
{
	IOCP_CONTEXT* context = NULL;
	context = xMalloc(sizeof(IOCP_CONTEXT));
	xMemSet(context, 0, sizeof(IOCP_CONTEXT));
	context->instPtr = iocp;
	return context;
}

void IOCP_FreeContext(IOCP* iocp, IOCP_CONTEXT* context)
{
	if (NULL == context){
		return;
	}
	xFree(context);
}

void IOCP_CleanOlp(IOCP* iocp, IOCP_OVERLAPPED* olp)
{
	olp->oppType = IOCP_NONE;
	if(olp->timer != NULL)
	{
		TimerQueue_DeleteTimer(&iocp->timerQueue, olp->timer, FALSE);
		olp->timer = NULL;
	}
	xMemSet(&olp->olp, 0, sizeof(OVERLAPPED));
	olp->buf = NULL;
	olp->len = 0;
	olp->realLen = 0;
	olp->param = NULL;
}

void IOCP_CloseHandle(IOCP* iocp, IOCP_CONTEXT *context)
{
	SOCKET socket = INVALID_SOCKET;
	if(context->type == IOCP_HANDLE_SOCKET)
	{
		socket = (SOCKET)(context->handle);
		shutdown(socket, SD_BOTH);
		closesocket(socket);
	}
	else
	{
		CloseHandle(context->handle);
	}
}

int IOCP_SessionTimeout(IOCP* iocp, IOCP_CONTEXT* context)
{
	if(context->sessionTimeo == 0)
	{
		return FALSE;
	}
	else
	{
		return getMs(iocp->frequency, now(iocp->frequency) - context->startCounter) >= context->sessionTimeo;
	}
}

IOCP_KEY IOCP_Add_F(IOCP* iocp, HANDLE f, unsigned long sessionTimeo, unsigned int readSpeedLmt, unsigned int writeSpeedLmt, int sync)
{
	return _IOCP_Add(iocp, f, sessionTimeo, readSpeedLmt, writeSpeedLmt, TRUE, sync);
}

IOCP_KEY IOCP_Add_S(IOCP* iocp, SOCKET s, unsigned long sessionTimeo, unsigned int readSpeedLmt, unsigned int writeSpeedLmt, int sync)
{
	return _IOCP_Add(iocp, (HANDLE)s, sessionTimeo, readSpeedLmt, writeSpeedLmt, FALSE, sync);
}

IOCP_KEY _IOCP_Add(IOCP* iocp, HANDLE h, unsigned long sessionTimeo, unsigned int readSpeedLmt, unsigned int writeSpeedLmt, int isFile, int sync)
{
	//IOCP_CONTEXT* context = iocp->allocContext(iocp);
	IOCP_CONTEXT* context = IOCP_AllocContext(iocp);
	int ret = TRUE;

	context->handle = h;
	if(isFile){
		context->type = IOCP_HANDLE_FILE;
	}
	else {
		context->type = IOCP_HANDLE_SOCKET;
	}
	context->lockPtr = NULL;
	context->readOlp.speedLmt = readSpeedLmt;
	context->writeOlp.speedLmt = writeSpeedLmt;
	context->startCounter = now(iocp->frequency);
	context->sessionTimeo = sessionTimeo;

	EnterCriticalSection(&iocp->cs);
	do
	{
		if( iocp->iocpHandle != CreateIoCompletionPort(h, iocp->iocpHandle, (ULONG_PTR)(context), 0))
		{
			ret = FALSE;
			break;
		}
		else
		{
			if(sync)
			{
				context->lockPtr = LockPool_Allocate(&iocp->lockPool);
			}
			else
			{
				context->lockPtr = NULL;
			}
			Map_Insert(iocp->contextMap,h, sizeof(HANDLE),context, sizeof(IOCP_CONTEXT*));
		}
	}while(FALSE);
	LeaveCriticalSection(&iocp->cs);

	if(ret)
	{
		return context;
	}
	else
	{
		//iocp->freeContext(iocp, context);
		IOCP_FreeContext(iocp, context);
		return IOCP_NULLKEY;
	}
}

SOCKET IOCP_GetSocket(IOCP* iocp, IOCP_KEY key)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(key);
	return (SOCKET)(context->handle);
}

int IOCP_Refresh(IOCP* iocp, IOCP_KEY key)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(key);
	int ret = FALSE;

	if(context->lockPtr){
		//context->lockPtr->wlock();
		Lock_Write(context->lockPtr);
	}
	if(context->readOlp.oppType != IOCP_NONE || context->writeOlp.oppType != IOCP_NONE || context->status != IOCP_NORMAL)
	{
	
	}
	else
	{
		context->startCounter = now(iocp->frequency);
		ret = TRUE;
	}
	if(context->lockPtr) {
		//context->lockPtr->unlock();
		Lock_Unlock(context->lockPtr);
	}

	return ret;
}

int IOCP_Busy(IOCP* iocp, IOCP_KEY key)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(key);
	int ret = FALSE;

	if(context->lockPtr)
	{
		//context->lockPtr->wlock();
		Lock_Write(context->lockPtr);
	}
	ret = context->readOlp.oppType != IOCP_NONE || context->writeOlp.oppType != IOCP_NONE;
	if(context->lockPtr) {
		//context->lockPtr->unlock();
		Lock_Unlock(context->lockPtr);
	}

	return ret;
}

int IOCP_Cancel(IOCP* iocp, IOCP_KEY key)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(key);
	int ret = IOCP_SUCESS;

	if(context->lockPtr){
		Lock_Write(context->lockPtr);
	}

	if(context->readOlp.oppType == IOCP_DELAY_READ)
	{
		context->readOlp.oppType = IOCP_NONE;
		TimerQueue_DeleteTimer(&iocp->timerQueue,context->readOlp.timer, FALSE);
		context->readOlp.timer = NULL;
	}
	if(context->writeOlp.oppType == IOCP_DELAY_WRITE)
	{
		context->writeOlp.oppType = IOCP_NONE;
		TimerQueue_DeleteTimer(&iocp->timerQueue,context->writeOlp.timer, FALSE);
		context->writeOlp.timer = NULL;
	}

	if(context->status == IOCP_NORMAL && (context->readOlp.oppType != IOCP_NONE || context->writeOlp.oppType != IOCP_NONE))
	{
		//iocp->closeHandle(iocp, context);
		IOCP_CloseHandle(iocp, context);
		context->status |= (IOCP_CANCELED | IOCP_HANDLE_CLOSED);
		ret = IOCP_PENDING;
	}
	if(context->lockPtr) {
		Lock_Unlock(context->lockPtr);
	}

	return ret;
}

int IOCP_Remove(IOCP* iocp, IOCP_KEY key)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(key);
	int isBusy = FALSE;

	if( !iocp->inited ) {
		return IOCP_UNINITIALIZED;
	}
	if(context->lockPtr) {
		Lock_Write(context->lockPtr);
	}
	if( context->readOlp.oppType != IOCP_NONE || context->writeOlp.oppType != IOCP_NONE )
	{
		isBusy = TRUE;
	}
	else
	{
		context->status |= IOCP_REMOVE;
	}
	if(context->lockPtr) {
		Lock_Unlock(context->lockPtr);
	}

	if(isBusy)
	{
		return IOCP_BUSY;
	}
	if(context->readOlp.timer != NULL)
	{
		TimerQueue_DeleteTimer(&iocp->timerQueue, context->readOlp.timer, TRUE);
		context->readOlp.timer = NULL;
	}
	if(context->writeOlp.timer != NULL)
	{
		TimerQueue_DeleteTimer(&iocp->timerQueue, context->writeOlp.timer, TRUE);
		context->writeOlp.timer = NULL;
	}

	if(!(context->status & IOCP_HANDLE_CLOSED))
	{
		//iocp->closeHandle(iocp, context);
		IOCP_CloseHandle(iocp, context);
		context->status |= IOCP_HANDLE_CLOSED;
	}
	EnterCriticalSection(&iocp->cs);
	Map_Remove(iocp->contextMap, context->handle);
	if(context->lockPtr)
	{
		LockPool_Recycle(context->lockPtr);
		context->lockPtr = NULL;
	}
	LeaveCriticalSection(&iocp->cs);
	//iocp->freeContext(iocp, context);
	IOCP_FreeContext(iocp , context);
	return IOCP_SUCESS;
}

void IOCP_OnIoFinished(IOCP* iocp, IOCP_CONTEXT* context, int result, IOCP_OVERLAPPED* iocpOlpPtr, int transfered)
{
	IOCP_Proc func = NULL;
	void* callbackParam = NULL;
	int flags = IOCP_NONE;
	unsigned char* buf = NULL;
	unsigned int len = 0;

	if(iocpOlpPtr->timer != NULL)
	{
		TimerQueue_DeleteTimer(&iocp->timerQueue ,iocpOlpPtr->timer, TRUE);
		iocpOlpPtr->timer = NULL;
	}
	if(context->lockPtr) {
		Lock_Read(context->lockPtr);
	}

	if(result) {
		iocpOlpPtr->transfered += transfered;
	}
	iocpOlpPtr->lastCompleteCounter = now(iocp->frequency);
	func = iocpOlpPtr->iocpProc;
	callbackParam = iocpOlpPtr->param;
	flags = iocpOlpPtr->oppType;
	buf = iocpOlpPtr->buf;
	len = iocpOlpPtr->len;

	//iocp->cleanOlp(iocp, iocpOlpPtr);
	IOCP_CleanOlp(iocp, iocpOlpPtr);

	if(context->status != IOCP_NORMAL)
	{
		flags |= context->status;
	}
	if(context->lockPtr) {;
		Lock_Unlock(context->lockPtr);
	}
	
	if(func)
	{
		func(context, flags, result, transfered, buf, len, callbackParam);
	}
}

int IOCP_Accept(IOCP* iocp, IOCP_KEY key, SOCKET sockNew, unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param)
{
	int ret = IOCP_PENDING;
	DWORD dwBytesReceived = 0;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	LPFN_ACCEPTEX lpfnAcceptEx = NULL;
	DWORD dwBytes = 0;

	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(key);
	SOCKET sockListen = (SOCKET)(context->handle);

	if( !iocp->inited ) {
		return IOCP_UNINITIALIZED;
	}
	if(len < (sizeof(SOCKADDR_IN) + 16) * 2) 
		return IOCP_BUFFERROR;

	if( 0 != WSAIoctl(sockListen, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &lpfnAcceptEx, sizeof(lpfnAcceptEx), &dwBytes, NULL, NULL) )
	{
		iocp->lastErrorCode = WSAGetLastError();
		return IOCP_UNDEFINED;
	}
	
	if(context->lockPtr) {
		Lock_Read(context->lockPtr);
	}
	if(context->status != IOCP_NORMAL)
	{
		ret = context->status; 
	}
	//else if(iocp->sessionTimeout(iocp, context))
	else if(IOCP_SessionTimeout(iocp, context))
	{
		ret = IOCP_SESSIONTIMEO;
	}
	else
	{
		context->readOlp.oppType = IOCP_ACCEPT;
		context->readOlp.buf = buf;
		context->readOlp.len = len;
		context->readOlp.realLen = len;
		context->readOlp.iocpProc = func;
		context->readOlp.param = param;
		context->readOlp.timeout = timeout;
		if(timeout > 0)
		{
			context->readOlp.timer = 
				//TimerQueue_CreateTimer(&iocp->timerQueue, timeout, iocp->readTimeout, context);
				TimerQueue_CreateTimer(&iocp->timerQueue, timeout, IOCP_ReadTimeout, context);
		}

		if( !lpfnAcceptEx(sockListen, sockNew, buf, 
			len - (sizeof(SOCKADDR_IN) + 16) * 2 , sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 
			&dwBytesReceived, (LPOVERLAPPED)(&context->readOlp)) )
		{
			if( WSA_IO_PENDING != WSAGetLastError())
			{
				iocp->lastErrorCode = WSAGetLastError();
				ret = IOCP_UNDEFINED;
			}
		}
		else
		{
			ret = IOCP_UNDEFINED;
		}
		if(ret != IOCP_PENDING)
		{
			//iocp->cleanOlp(iocp, &context->readOlp);
			IOCP_CleanOlp(iocp, &context->readOlp);
		}
	}

	if(context->lockPtr) {
		Lock_Unlock(context->lockPtr);
	}

	return ret;
}

int IOCP_RealRecv(IOCP* iocp, IOCP_CONTEXT* context)
{
	DWORD dwTransfered = 0;
	DWORD dwLastError = 0;
	DWORD dwFlags = 0;
	WSABUF wsaBuf;
	SOCKET socket = INVALID_SOCKET;

	if(context->type == IOCP_HANDLE_SOCKET)
	{
		wsaBuf.len = context->readOlp.len;
		wsaBuf.buf = (char*)(context->readOlp.buf);

		socket = (SOCKET)(context->handle);
		if(SOCKET_ERROR == WSARecv(socket, &wsaBuf, 1, &dwTransfered, &dwFlags, (LPWSAOVERLAPPED)(&context->readOlp), NULL))
		{
			dwLastError = WSAGetLastError();
			if(dwLastError != WSA_IO_PENDING)
			{
				iocp->lastErrorCode = dwLastError;
				return IOCP_UNDEFINED;
			}
		}
		return IOCP_PENDING;
	}
	else
	{
		if(!ReadFile(context->handle, context->readOlp.buf, context->readOlp.len, &dwTransfered, (LPOVERLAPPED)(&context->readOlp)))
		{
			dwLastError = GetLastError();
			if(dwLastError != ERROR_IO_PENDING)
			{
				iocp->lastErrorCode = dwLastError;
				return IOCP_UNDEFINED;
			}
		}
		return IOCP_PENDING;
	}
}

int IOCP_Read(IOCP* iocp, IOCP_KEY key, unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param)
{
	return IOCP_Recv(iocp, key, buf, len, timeout, func, param);
}

int IOCP_Recv(IOCP* iocp, IOCP_KEY key, unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(key);
	int ret = IOCP_PENDING;
	unsigned int delay = 0;
	__int64 expectTime = 0;

	if( !iocp->inited ) {
		return IOCP_UNINITIALIZED;
	}

	if(context->lockPtr) {
		Lock_Read(context->lockPtr);
	}
	if(context->status != IOCP_NORMAL)
	{
		ret = context->status; 
	}
	//else if(iocp->sessionTimeout(iocp, context))
	else if(IOCP_SessionTimeout(iocp, context))
	{
		ret = IOCP_SESSIONTIMEO;
	}
	else
	{
		context->readOlp.oppType = IOCP_RECV;
		context->readOlp.buf = buf;
		context->readOlp.len = len;
		context->readOlp.timeout = timeout;
		context->readOlp.iocpProc = func;
		context->readOlp.param = param;
		context->readOlp.realLen = len;

		if(context->readOlp.speedLmt > 0 && context->readOlp.lastCompleteCounter != 0)
		{
			__int64 expectTime = getCounters(iocp->frequency, (__int64)(context->readOlp.transfered * 1.0 / context->readOlp.speedLmt * 1000));
			expectTime += context->startCounter;

			if(expectTime > context->readOlp.lastCompleteCounter)
			{
				delay = (unsigned int)(getMs(iocp->frequency, expectTime - context->readOlp.lastCompleteCounter));
				if(delay > IOCP_MAXWAITTIME_ONSPEEDLMT)
				{
					delay = IOCP_MAXWAITTIME_ONSPEEDLMT;
					if(context->readOlp.len > IOCP_MINBUFLEN_ONSPEEDLMT)
					{
						context->readOlp.realLen = IOCP_MINBUFLEN_ONSPEEDLMT;
					}
				}
				else
				{
					context->readOlp.realLen = context->readOlp.len;
				}
			}
		}
		if(delay > 0)
		{
			context->readOlp.oppType = IOCP_DELAY_READ;
			context->readOlp.timer = 
				//TimerQueue_CreateTimer(&iocp->timerQueue, delay, iocp->delayRecv, context);
				TimerQueue_CreateTimer(&iocp->timerQueue, delay, IOCP_DelayRecv, context);
		}
		else
		{
			if(context->readOlp.timeout > 0)
			{
				context->readOlp.timer = 
					//TimerQueue_CreateTimer(&iocp->timerQueue, context->readOlp.timeout, iocp->readTimeout, context);
					TimerQueue_CreateTimer(&iocp->timerQueue, context->readOlp.timeout, IOCP_ReadTimeout, context);

			}

			//ret = iocp->realRecv(iocp, context);
			ret = IOCP_RealRecv(iocp, context);

			if(IOCP_PENDING != ret)
			{
				//iocp->cleanOlp(iocp, &context->readOlp);
				IOCP_CleanOlp(iocp, &context->readOlp);
			}
		}
	}
	if(context->lockPtr) {
		Lock_Unlock(context->lockPtr);
	}

	return ret;
}

int IOCP_RealSend(IOCP* iocp, IOCP_CONTEXT* context)
{
	DWORD dwTransfered = 0;
	DWORD dwLastError = 0;

	if(context->type == IOCP_HANDLE_SOCKET)
	{
		WSABUF wsaBuf = { context->writeOlp.realLen, (char*)(context->writeOlp.buf) };
		SOCKET s = (SOCKET)(context->handle);
		if(SOCKET_ERROR == WSASend(s, &wsaBuf, 1, &dwTransfered, 0, (LPWSAOVERLAPPED)(&context->writeOlp), NULL))
		{
			dwLastError = WSAGetLastError();
			if(dwLastError != WSA_IO_PENDING)
			{
				iocp->lastErrorCode = dwLastError;
				return IOCP_UNDEFINED;
			}
		}
		return IOCP_PENDING;
	}
	else
	{
		if(!WriteFile(context->handle, context->writeOlp.buf, context->writeOlp.len, &dwTransfered, (LPOVERLAPPED)(&context->writeOlp)))
		{
			dwLastError = GetLastError();
			if(dwLastError != ERROR_IO_PENDING)
			{
				iocp->lastErrorCode = dwLastError;
				return IOCP_UNDEFINED;
			}
		}
		return IOCP_PENDING;
	}
}

int IOCP_Write(IOCP* iocp, IOCP_KEY key, const unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param)
{
	return IOCP_Send(iocp, key, buf, len, timeout, func, param);
}

int IOCP_Send(IOCP* iocp, IOCP_KEY key, const unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(key);
	int ret = IOCP_PENDING;
	unsigned int delay = 0;
	__int64 expectTime;

	if( !iocp->inited ) {
		return IOCP_UNINITIALIZED;
	}

	if(context->lockPtr) {
		Lock_Read(context->lockPtr);
	}
	if(context->status != IOCP_NORMAL)
	{
		ret = context->status; 
	}
	//else if(iocp->sessionTimeout(iocp, context))
	else if(IOCP_SessionTimeout(iocp, context))
	{
		ret = IOCP_SESSIONTIMEO;
	}
	else
	{
		context->writeOlp.oppType = IOCP_SEND;
		context->writeOlp.buf = (unsigned char*)(buf);
		context->writeOlp.len = len;
		context->writeOlp.timeout = timeout;
		context->writeOlp.iocpProc = func;
		context->writeOlp.param = param;
		context->writeOlp.realLen = len;

		if(context->writeOlp.speedLmt > 0 && context->writeOlp.lastCompleteCounter != 0)
		{
			expectTime = getCounters(iocp->frequency, (__int64)(context->writeOlp.transfered * 1.0 / context->writeOlp.speedLmt * 1000));
			expectTime += context->startCounter;

			if(expectTime > context->writeOlp.lastCompleteCounter)
			{
				delay = (unsigned int)(getMs(iocp->frequency, expectTime - context->writeOlp.lastCompleteCounter));
				if(delay > IOCP_MAXWAITTIME_ONSPEEDLMT)
				{
					delay = IOCP_MAXWAITTIME_ONSPEEDLMT;
					if(context->writeOlp.len > IOCP_MINBUFLEN_ONSPEEDLMT)
					{
						context->writeOlp.realLen = IOCP_MINBUFLEN_ONSPEEDLMT;
					}
				}
				else
				{
					context->writeOlp.realLen = context->writeOlp.len;
				}

			}
		}

		if(delay > 0)
		{
			context->writeOlp.oppType = IOCP_DELAY_WRITE;
			context->writeOlp.timer = 
				//TimerQueue_CreateTimer(&iocp->timerQueue, delay, iocp->delaySend, context);
				TimerQueue_CreateTimer(&iocp->timerQueue, delay, IOCP_DelaySend, context);
		}
		else
		{
			if(context->writeOlp.timeout > 0)
			{
				context->writeOlp.timer = 
					//TimerQueue_CreateTimer(&iocp->timerQueue, context->writeOlp.timeout, iocp->readTimeout, context);
					TimerQueue_CreateTimer(&iocp->timerQueue, context->writeOlp.timeout, IOCP_ReadTimeout, context);
			}

			//ret = iocp->realSend(iocp, context);
			ret = IOCP_RealSend(iocp, context);

			if( ret != IOCP_PENDING)
			{
				//iocp->cleanOlp(iocp, &context->writeOlp);
				IOCP_CleanOlp(iocp, &context->writeOlp);
			}
		}
	}
	if(context->lockPtr) {
		Lock_Unlock(context->lockPtr);
	}
	
	return ret;
}

int IOCP_Connect(IOCP* iocp, IOCP_KEY key, SOCKADDR* addr, unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(key);
	SOCKET socket = (SOCKET)(context->handle);
	DWORD dwBytesReceived = 0;
	GUID GuidConnectEx = WSAID_CONNECTEX;
	LPFN_CONNECTEX lpfnConnectEx = NULL;
	DWORD dwBytes = 0;
	int ret = IOCP_PENDING;
	DWORD bytesSent = 0;

	if( !iocp->inited ) {
		return IOCP_UNINITIALIZED;
	}
	if( 0 != WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &lpfnConnectEx, sizeof(lpfnConnectEx), &dwBytes, NULL, NULL) )
	{
		iocp->lastErrorCode = WSAGetLastError();
		return IOCP_UNDEFINED;
	}
	if(context->lockPtr) {
		Lock_Read(context->lockPtr);
	}
	if(context->status != IOCP_NORMAL)
	{
		ret = context->status; 
	}
	//else if(iocp->sessionTimeout(iocp, context))
	else if(IOCP_SessionTimeout(iocp, context))
	{
		ret = IOCP_SESSIONTIMEO;
	}
	else
	{
		context->writeOlp.oppType = IOCP_CONNECT;
		context->writeOlp.buf = buf;
		context->writeOlp.len = len;
		context->writeOlp.iocpProc = func;
		context->writeOlp.param = param;
		context->writeOlp.realLen = len;
		context->writeOlp.timeout = timeout;

		if(timeout > 0)
		{
			context->writeOlp.timer = 
				//TimerQueue_CreateTimer(&iocp->timerQueue, timeout , iocp->writeTimeout, context);
				TimerQueue_CreateTimer(&iocp->timerQueue, timeout , IOCP_WriteTimeout, context);
		}

		if( !lpfnConnectEx(socket, addr, sizeof(SOCKADDR), buf, len, &bytesSent, (LPOVERLAPPED)(&context->writeOlp)) )
		{
			int errorCode = WSAGetLastError();
			if( WSA_IO_PENDING != errorCode )
			{
				iocp->lastErrorCode = errorCode;
				ret = IOCP_UNDEFINED;
			}
		}
		else
		{
			ret = IOCP_UNDEFINED;
		}

		if(ret != IOCP_PENDING)
		{
			//iocp->cleanOlp(iocp, &context->writeOlp);
			IOCP_CleanOlp(iocp, &context->writeOlp);
		}
	}
	if(context->lockPtr) {
		Lock_Unlock(context->lockPtr);
	}

	return ret;
}

void WINAPI IOCP_ReadTimeout(void* param)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(param);

	if(context->lockPtr) {
		Lock_Write(context->lockPtr);
	}
	if(context->readOlp.oppType != IOCP_NONE && context->status == IOCP_NORMAL)
	{
		//context->instPtr->closeHandle(context->instPtr, context);
		IOCP_CloseHandle(context->instPtr, context);
		context->status |= (IOCP_READTIMEO | IOCP_HANDLE_CLOSED);
	}
	if(context->lockPtr) {
		Lock_Unlock(context->lockPtr);
	}
}

void WINAPI IOCP_WriteTimeout(void* param)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(param);

	if(context->lockPtr) {
		Lock_Write(context->lockPtr);
	}
	if(context->writeOlp.oppType != IOCP_NONE && context->status == IOCP_NORMAL)
	{
		//context->instPtr->closeHandle(context->instPtr, context);
		IOCP_CloseHandle(context->instPtr, context);
		context->status |= (IOCP_WRITETIMEO | IOCP_HANDLE_CLOSED);
	}
	if(context->lockPtr) {
		Lock_Unlock(context->lockPtr);
	}
}

void WINAPI IOCP_DelaySend(void* param)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(param);
	IOCP_Proc func = NULL;
	void* callbackParam = NULL;
	int flags = IOCP_NONE;
	unsigned char* buf = NULL;
	unsigned int len = 0;
	int ret = IOCP_PENDING;

	if(context->writeOlp.timer)
	{
		TimerQueue_DeleteTimer(&context->instPtr->timerQueue,context->writeOlp.timer, FALSE);
		context->writeOlp.timer = NULL;
	}

	if(context->lockPtr) {
		Lock_Read(context->lockPtr);
	}
	if(context->status != IOCP_NORMAL)
	{
		ret = IOCP_BUSY;
	}
	else
	{
		if(context->writeOlp.oppType != IOCP_DELAY_WRITE){

		}
		else
		{
			context->writeOlp.oppType = IOCP_SEND;
			if(context->writeOlp.timeout > 0)
			{
				context->writeOlp.timer = 
					//TimerQueue_CreateTimer(&context->instPtr->timerQueue, context->writeOlp.timeout, iocp->writeTimeoutProc, context);
					TimerQueue_CreateTimer(&context->instPtr->timerQueue, context->writeOlp.timeout, IOCP_WriteTimeout, context);
			}
			//ret =  context->instPtr->realSend(context->instPtr, context);
			ret = IOCP_RealSend(context->instPtr, context);

			if(IOCP_PENDING != ret)
			{
				func = context->writeOlp.iocpProc;
				callbackParam = context->writeOlp.param;
				flags = context->writeOlp.oppType;
				buf = context->writeOlp.buf;
				len = context->writeOlp.len;

				//context->instPtr->cleanOlp(context->instPtr, &context->writeOlp);
				IOCP_CleanOlp(context->instPtr, &context->writeOlp);
			}
		}
	}
	if(context->lockPtr) {
		Lock_Unlock(context->lockPtr);
	}

	if(func)
	{
		func(context, flags, FALSE, 0, buf, len, callbackParam);
	}
}

void WINAPI IOCP_DelayRecv(void* param)
{
	IOCP_CONTEXT* context = (IOCP_CONTEXT*)(param);
	IOCP_Proc func = NULL;
	void* callbackParam = NULL;
	int flags = IOCP_NONE;
	unsigned char* buf = NULL;
	unsigned int len = 0;
	int ret = IOCP_PENDING;
	if(context->readOlp.timer)
	{
		TimerQueue_DeleteTimer(&context->instPtr->timerQueue,context->readOlp.timer, FALSE);
		context->readOlp.timer = NULL;
	}

	if(context->lockPtr) {
		Lock_Read(context->lockPtr);
	}
	if(context->status != IOCP_NORMAL)
	{
		ret = IOCP_BUSY;
	}
	else
	{
		if(context->readOlp.oppType != IOCP_DELAY_READ)
		{
		}
		else
		{
			context->readOlp.oppType = IOCP_RECV;
			if(context->readOlp.timeout > 0)
			{
				context->readOlp.timer = 
					//TimerQueue_CreateTimer(&context->instPtr->timerQueue, context->readOlp.timeout,context->instPtr->readTimeout, context);
					TimerQueue_CreateTimer(&context->instPtr->timerQueue, context->readOlp.timeout,IOCP_ReadTimeout, context);
			}

			//ret =  context->instPtr->realRecv(context->instPtr, context);
			ret =  IOCP_RealRecv(context->instPtr, context);

			if(IOCP_PENDING != ret)
			{
				func = context->readOlp.iocpProc;
				callbackParam = context->readOlp.param;
				flags = context->readOlp.oppType;
				buf = context->readOlp.buf;
				len = context->readOlp.len;

				//context->instPtr->cleanOlp(context->instPtr, &context->readOlp);
				IOCP_CleanOlp(context->instPtr, &context->readOlp);
			}
		}
	}
	if(context->lockPtr) {
		Lock_Unlock(context->lockPtr);
	}

	if(func)
	{
		func(context, flags, FALSE, 0, buf, len, callbackParam);
	}
}

DWORD WINAPI IOCP_Service(void* lpParam)
{
	IOCP* instPtr = (IOCP*)(lpParam);
	DWORD transfered = 0;
	IOCP_CONTEXT* context = NULL;
	IOCP_OVERLAPPED* iocpOlpPtr = NULL;

	while(TRUE)
	{
		if(!GetQueuedCompletionStatus(instPtr->iocpHandle, &transfered, (PULONG_PTR)(&context), (LPOVERLAPPED*)(&iocpOlpPtr), INFINITE))
		{
			if(iocpOlpPtr)
			{
				//instPtr->onIoFinished(instPtr, context, FALSE, iocpOlpPtr, transfered);
				IOCP_OnIoFinished(instPtr, context, FALSE, iocpOlpPtr, transfered);
			}
			else
			{
				instPtr->lastErrorCode = GetLastError();
				return IOCP_UNDEFINED;
			}
		}
		else
		{
			if(transfered == 0 && iocpOlpPtr == NULL && context == NULL)
			{
				break;
			}
			//instPtr->onIoFinished(instPtr, context, TRUE, iocpOlpPtr, transfered);
			IOCP_OnIoFinished(instPtr, context, TRUE, iocpOlpPtr, transfered);
		}
	}
	return 0;
}
