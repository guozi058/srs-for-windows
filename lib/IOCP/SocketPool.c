#include "SocketPool.h"
#include "Util.h"

SocketPool* SocketPool_New()
{
	SocketPool* socketpool = NULL;
	socketpool = xMalloc(sizeof(SocketPool));
	if (NULL == socketpool){
		return NULL;
	}
	InitializeCriticalSection(&socketpool->cs);
	socketpool->socketList = NULL;

	return socketpool;
}