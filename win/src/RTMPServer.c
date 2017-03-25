#include "RTMPServer.h"
#include <WinSock2.h>
#include <MSWSock.h>

RtmpServer* RtmpServer_New(unsigned short port)
{
	RtmpServer* rs = NULL;
	rs = xMalloc(sizeof(RtmpServer));
	if (NULL == rs){
		return NULL;
	}
	rs->acceptcontex.len = (sizeof(SOCKADDR_IN) + 16) * 2;  
	rs->acceptcontex.buf = xMalloc(rs->acceptcontex.len);

	rs->iocp = NULL;
	rs->socket = INVALID_SOCKET;
	rs->client = INVALID_SOCKET;
	rs->listen = IOCP_NULLKEY;
	rs->port = port;
	
	return rs;
}

int RtmpServer_Destroy(RtmpServer* rs)
{
	xFree(rs->acceptcontex.buf);
}

int RtmpServer_Init(RtmpServer* rs)
{
	int ret;
	
	rs->iocp = IOCP_New();
	if (NULL == rs->iocp){
		return;
	}
}

int RtmpServer_Listen(RtmpServer* rs)
{
	unsigned long nonblock = 1;
	SOCKADDR_IN serviceAddr;
	int iOptVal;
	int iOptLen = sizeof(int);
	int ret = ERROR_SUCCESS;

	if( (rs->socket = socket(PF_INET, SOCK_STREAM, 0 )) == INVALID_SOCKET ){
		ret = ERROR_SOCKET_CREATE;
		return ret;
	}
	if (setsockopt(rs->socket, SOL_SOCKET, SO_REUSEADDR, (char*)&iOptVal, sizeof(int)) == SOCKET_ERROR) {
		ret = ERROR_SOCKET_SETREUSE;
		return ret;
	}

	ioctlsocket(rs->socket, FIONBIO, &nonblock);
	
	serviceAddr.sin_family	= AF_INET;
	serviceAddr.sin_port = htons(rs->port);
	serviceAddr.sin_addr.s_addr = INADDR_ANY;
	if(bind(rs->socket, (SOCKADDR*)&serviceAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		closesocket(rs->socket);
		ret = ERROR_SOCKET_BIND;
		return ret;
	}

	if(listen(rs->socket, SERVER_LISTEN_BACKLOG) == -1)
	{
		closesocket(rs->socket);
		ret = ERROR_SOCKET_LISTEN;
		return ret;
	}
	return ret;
}

int RtmpServer_Run(RtmpServer* rs)
{
	int ret;

	if(IOCP_SUCESS != IOCP_Init(rs->iocp, 0)){
		return;
	}
	RtmpServer_Listen(rs);
	rs->listen = IOCP_Add_S(rs->iocp,rs->socket,0,0,0 ,FALSE);
	if (IOCP_NULLKEY == rs->listen){
		return;
	}
}

int RtmpServer_DoAccept(RtmpServer* rs)
{
	int ret;
	if (IOCP_NULLKEY == rs->listen){
		return ;
	}
	rs->client = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if( INVALID_SOCKET == rs->client)
	{
		return ERROR_SOCKET_CREATE;
	}

	//if(IOCP_PENDING != IOCP_Accept(rs->iocp, rs->listen, rs->client, rs->acceptcontex.buf, rs->acceptcontex.len, 0, rs->acceptCallback, rs))
	if(IOCP_PENDING != IOCP_Accept(rs->iocp, rs->listen, rs->client, rs->acceptcontex.buf, rs->acceptcontex.len, 0, RtmpServer_AcceptCallback, rs))
	{
		closesocket(rs->client);
		rs->client = INVALID_SOCKET;
		return ;
	}
	else
	{
		return ;
	}

}

void RtmpServer_AcceptCallback(IOCP_KEY s, int flags, int result, int transfered, unsigned char* buf, unsigned int len, void* param)
{
	RtmpServer* rs = (RtmpServer*)(param);

	if (flags & IOCP_ACCEPT){
		//rs->onAccept(rs, result);
		RtmpServer_OnAccept(rs, result);
	}
	else
		return;
}

void RtmpServer_OnAccept(RtmpServer* rs, int sucess)
{
	SOCKET sockListen = INVALID_SOCKET;
	SOCKET sNewClient = INVALID_SOCKET;
	SOCKADDR_IN clientAddr;
	unsigned short clientPort = 0;
	int nAddrLen = 0;

	if(!sucess)
	{
		shutdown(rs->client, SD_BOTH);
		closesocket(rs->client);
		rs->client = INVALID_SOCKET;
		//rs->doAccept(rs);
		RtmpServer_DoAccept(rs);
		return;
	}

	sockListen = IOCP_GetSocket(rs->iocp, rs->listen);
	if( 0 != setsockopt( rs->client, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&sockListen, sizeof(sockListen)) ){
	}

	nAddrLen = sizeof(SOCKADDR_IN);
	if( 0 != getpeername(rs->client, (SOCKADDR *)&clientAddr, &nAddrLen) )
	{
	}
	//std::string clientIp = inet_ntoa(clientAddr.sin_addr);
	clientPort = ntohs(clientAddr.sin_port);

	sNewClient = rs->client;
	rs->client = INVALID_SOCKET;

/*
	connection_context_t* conn = NULL;
	int kicked = false;
	int refused = false;

	_lock.lock();
	do 
	{
		if(_connections.size() >= this->maxConnections())
		{
			refused = true;
			break;
		}

		str_int_map_t::iterator iterClientIP = _connectionIps.end();
		if(this->maxConnectionsPerIp() > 0)
		{
			iterClientIP = _connectionIps.find(clientIp);
			if(iterClientIP != _connectionIps.end() && iterClientIP->second >= this->maxConnectionsPerIp())
			{
				kicked = true;
				break;
			}
		}

		conn = allocConnectionContext(clientIp, clientPort);
		conn->clientSock = _network.add(sNewClient, this->sessionTimeout(), 0, this->maxConnectionSpeed());
		_connections.insert(std::make_pair(conn->clientSock, conn));
		if(this->maxConnectionsPerIp() > 0)
		{
			if(iterClientIP != _connectionIps.end()) iterClientIP->second++;
			else _connectionIps.insert(std::make_pair(clientIp, 1));
		}
	} while(FALSE);
	_lock.unlock();

	if( conn != NULL )
	{
		int ret = conn->request->run(conn, conn->clientSock, recvTimeout());
		if(CT_SUCESS != ret)
		{
			doConnectionClosed(conn, ret);
		}
		else
		{

		}
	}
	else
	{
		if(refused)
		{
		}
		else if(kicked)
		{
		}
		else
		{
		}
		shutdown(sNewClient, SD_BOTH);
		closesocket(sNewClient);
	}
	*/
	RtmpServer_DoAccept(rs);
}
