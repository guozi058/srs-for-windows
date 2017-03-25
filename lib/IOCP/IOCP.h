#ifndef _IOCP_H_
#define _IOCP_H_

#include "TimerQueue.h"
#include "LockPool.h"
#include "Map.h"
#include <WinSock2.h>

typedef enum {
	IOCP_BUFFERROR = 0x0001,
	IOCP_PENDING = 0x0002,
	IOCP_UNINITIALIZED = 0x0004,
	IOCP_DESTROYFAILED = 0x0008,
	IOCP_READTIMEO = 0x0010,
	IOCP_SEND = 0x0020,
	IOCP_RECV = 0x0040,
	IOCP_CONNECT = 0x0080,
	IOCP_ACCEPT = 0x0100,
	IOCP_CANCELED = 0x0200,
	IOCP_BUSY = 0x0400,
	IOCP_REMOVE = 0x0800,
	IOCP_WRITETIMEO = 0x1000,
	IOCP_HANDLE_CLOSED = 0x2000,
	IOCP_DELAY_READ = 0x4000,
	IOCP_DELAY_WRITE = 0x8000,
	IOCP_SESSIONTIMEO = 0x010000,
	IOCP_ALL = 0x7FFFFFFF,
	IOCP_UNDEFINED = 0xFFFFFFFF
}IOCP_PARA;

#define IOCP_NULLKEY NULL
#define IOCP_HANDLE_SOCKET 1
#define IOCP_HANDLE_FILE 2
#define IOCP_SUCESS 0
#define IOCP_NONE 0
#define IOCP_NORMAL 0
#define IOCP_MAXWAITTIME_ONSPEEDLMT 2000
#define IOCP_MINBUFLEN_ONSPEEDLMT 512

typedef void* IOCP_KEY;
typedef void (*IOCP_Proc)(IOCP_KEY k, int flags, int result, int transfered, unsigned char* buf, unsigned int len, void* param); 

typedef struct IOCP_OVERLAPPED_T
{
	OVERLAPPED olp;
	int oppType;
	unsigned char* buf;
	unsigned int len;
	unsigned int realLen;
	IOCP_Proc iocpProc;
	void* param;
	Timer timer;
	unsigned int timeout;
	unsigned int speedLmt;
	__int64 lastCompleteCounter;
	__int64 transfered;
}IOCP_OVERLAPPED;

typedef struct _IOCP_T IOCP;

typedef struct IOCP_CONTEXT_T
{
	HANDLE handle;
	IOCP_OVERLAPPED readOlp;
	IOCP_OVERLAPPED writeOlp;
	Lock* lockPtr;
	int status;
	__int64 startCounter;
	unsigned long sessionTimeo;
	IOCP* instPtr;
	int type;
}IOCP_CONTEXT;

typedef struct _IOCP_T
{
	HANDLE iocpHandle;
	int threads;
	HANDLE* hThreads;
	int inited; 
	DWORD lastErrorCode;
	Map* contextMap;
	CRITICAL_SECTION cs;
	TimerQueue timerQueue;
	__int64 frequency;
	LockPool lockPool;
}IOCP;

#ifdef __cplusplus
extern "C"
{
#endif


#ifndef WSAID_ACCEPTEX
#define WSAID_ACCEPTEX {0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
typedef BOOL (PASCAL *LPFN_ACCEPTEX)(SOCKET,SOCKET,PVOID,DWORD,DWORD,DWORD,LPDWORD,LPOVERLAPPED);
#endif
#ifndef WSAID_CONNECTEX
#define WSAID_CONNECTEX {0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}}
typedef BOOL (PASCAL *LPFN_CONNECTEX) (SOCKET,const SOCKADDR*,int,PVOID,DWORD,LPDWORD,LPOVERLAPPED);
#endif

int InitWinsock(WORD nMainVer, WORD nSubVer);
int CleanWinsock();

IOCP* IOCP_New(void);
int IOCP_Init(IOCP* iocp, int threads);
int IOCP_Destroy(IOCP* iocp);
IOCP_CONTEXT* IOCP_AllocContext(IOCP* iocp);
void IOCP_FreeContext(IOCP* iocp, IOCP_CONTEXT* context);
void IOCP_CleanOlp(IOCP* iocp, IOCP_OVERLAPPED* olp);
void IOCP_CloseHandle(IOCP* iocp, IOCP_CONTEXT *context);
int IOCP_SessionTimeout(IOCP* iocp, IOCP_CONTEXT* context);
IOCP_KEY IOCP_Add_F(IOCP* iocp, HANDLE f, unsigned long sessionTimeo, unsigned int readSpeedLmt, unsigned int writeSpeedLmt, int sync);
IOCP_KEY IOCP_Add_S(IOCP* iocp, SOCKET s, unsigned long sessionTimeo, unsigned int readSpeedLmt, unsigned int writeSpeedLmt, int sync);
IOCP_KEY _IOCP_Add(IOCP* iocp, HANDLE h, unsigned long sessionTimeo, unsigned int readSpeedLmt, unsigned int writeSpeedLmt, int isFile, int sync);
SOCKET IOCP_GetSocket(IOCP* iocp, IOCP_KEY key);
int IOCP_Refresh(IOCP* iocp, IOCP_KEY key);
int IOCP_Busy(IOCP* iocp, IOCP_KEY key);
int IOCP_Cancel(IOCP* iocp, IOCP_KEY key);
int IOCP_Remove(IOCP* iocp, IOCP_KEY key);

void IOCP_OnIoFinished(IOCP* iocp, IOCP_CONTEXT* context, int result, IOCP_OVERLAPPED* iocpOlpPtr, int transfered);
int IOCP_Accept(IOCP* iocp, IOCP_KEY key, SOCKET sockNew, unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param);
int IOCP_RealRecv(IOCP* iocp, IOCP_CONTEXT* context);
int IOCP_Read(IOCP* iocp, IOCP_KEY key, unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param);
int IOCP_Recv(IOCP* iocp, IOCP_KEY key, unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param);
int IOCP_RealSend(IOCP* iocp, IOCP_CONTEXT* context);
int IOCP_Write(IOCP* iocp, IOCP_KEY key, const unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param);
int IOCP_Send(IOCP* iocp, IOCP_KEY key, const unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param);
int IOCP_Connect(IOCP* iocp, IOCP_KEY key, SOCKADDR* addr, unsigned char* buf, unsigned int len, unsigned int timeout, IOCP_Proc func, void* param);

void WINAPI IOCP_ReadTimeout(void* param);
void WINAPI IOCP_WriteTimeout(void* param);
void WINAPI IOCP_DelaySend(void* param);
void WINAPI IOCP_DelayRecv(void* param);

DWORD WINAPI IOCP_Service(void* lpParam);

#ifdef __cplusplus
}
#endif

#endif
