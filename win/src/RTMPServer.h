#ifndef _RTMPSERVER_H_
#define _RTMPSERVER_H_

#include "IOCP.h"

#define ERROR_SOCKET_CREATE                 200
#define ERROR_SOCKET_SETREUSE               201
#define ERROR_SOCKET_BIND                   202
#define ERROR_SOCKET_LISTEN                 203
#define SERVER_LISTEN_BACKLOG				512

typedef struct ACCEPT_CONTEXT_T
{
	unsigned char* buf;
	unsigned int len;
}ACCEPT_CONTEXT;

typedef struct _RTMPSERVER_T
{
	IOCP* iocp;
	IOCP_KEY listen;
	SOCKET socket;
	SOCKET client;
	ACCEPT_CONTEXT acceptcontex;
	unsigned short port;
}RtmpServer;

#ifdef __cplusplus
extern "C"
{
#endif

RtmpServer* RtmpServer_New(unsigned short port);
int RtmpServer_Init(RtmpServer* rs);
int RtmpServer_Listen(RtmpServer* rs);
int RtmpServer_Run(RtmpServer* rs);
int RtmpServer_DoAccept(RtmpServer* rs);
void RtmpServer_OnAccept(RtmpServer* rs);

void RtmpServer_AcceptCallback(IOCP_KEY s, int flags, int result, int transfered, unsigned char* buf, unsigned int len, void* param);

#ifdef __cplusplus
}
#endif

#endif
