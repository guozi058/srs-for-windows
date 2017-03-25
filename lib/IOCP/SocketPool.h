#ifndef _SOCKETPOOL_H_
#define _SOCKETPOOL_H_

#include <WinSock2.h>

typedef struct SOCKET_T
{
	SOCKET socket;
	int isFree;
	int index;
}Socket;

typedef struct _SOCKETPOOL_T
{
	Socket* socketList;
	CRITICAL_SECTION cs;

}SocketPool;

#ifdef __cplusplus
extern "C"
{
#endif

SocketPool* SocketPool_New();
int SocketPool_Init(SocketPool*,unsigned int);

#ifdef __cplusplus
}
#endif

#endif
