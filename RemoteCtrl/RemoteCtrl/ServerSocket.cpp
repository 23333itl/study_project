#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;
CServerSocket* CServerSocket::m_instance = nullptr;
//CServerSocket* pserver = CServerSocket::getInstance();
CServerSocket::Helper CServerSocket::m_helper;
