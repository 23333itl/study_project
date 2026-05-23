#pragma once
#include "pch.h"
#include "framework.h"

class CServerSocket
{
public:
	static CServerSocket* getInstance(){
		if (m_instance == nullptr) {//静态函数没有this指针
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	bool InitSocket() {
		//TODO:校验
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(9527);
		//bind
		if (bind(ser_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
			return false;
		}
		
		//listen
		if (listen(ser_sock, 1) == -1) {
			return false;
		}
		return true;
		
		
	}
	bool AcceptClient() {
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client=accept(ser_sock, (sockaddr*) & client_adr, &cli_sz);
		if (m_client == -1) {
			return false;
		}
		return true;
		//recv(client, buffer, sizeof(buffer), 0);
		//send(client, buffer, sizeof(buffer),0);
		
	}

	int  DealCommand() {
		char buffer[1024];
		while (true) {
			memset(buffer, 0, sizeof(buffer));
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0) {
				return -1;
			}
			//TODO:解析命令并执行
		}
	}

	int Send(const char* pData, int nSize) {
		if (m_client == -1)   return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
private:
	SOCKET ser_sock;
	SOCKET m_client;
	CServerSocket(const CServerSocket& ss){
		ser_sock = ss.ser_sock;
		m_client = ss.m_client;
	}
	CServerSocket& operator=(const CServerSocket& ss){}
	CServerSocket() {
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"),_T( "初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		//套接字初始化
		ser_sock = socket(PF_INET, SOCK_STREAM, 0);

	}
	~CServerSocket() {
		closesocket(ser_sock);
		WSACleanup();

	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
	static void releaseinstance() {
		if (m_instance != nullptr) {
			CServerSocket* tmp = m_instance;
			m_instance = nullptr;
			delete tmp;
		}
	}

	static CServerSocket* m_instance;
	class Helper {
	public:
		Helper() {
			CServerSocket::getInstance();
		}
		~Helper() {
			CServerSocket::releaseinstance();
		}
	};
	static Helper m_helper;
};


