#include "pch.h"
#include "ClientSocket.h"

//CClientSocket server;
CClientSocket* CClientSocket::m_instance = nullptr;
//CClientSocket* pclient = CClientSocket::getInstance();
CClientSocket::Helper CClientSocket::m_helper;

std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}