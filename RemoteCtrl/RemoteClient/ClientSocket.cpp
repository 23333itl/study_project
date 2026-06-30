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

//查看具体原始数据
void Dump(BYTE* pData, size_t nSize) {
	std::string strOUT;
	for (size_t i = 0;i < nSize;i++) {
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0)) strOUT += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
		strOUT += buf;
	}
	strOUT += "\n";
	OutputDebugStringA(strOUT.c_str());
}