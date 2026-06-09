#pragma once
#include "pch.h"
#include "framework.h"
#include<string>
#include<vector>

#pragma pack(push)
#pragma pack(1)
#define BUFFER_SIZE 409600*2

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否无效
	BOOL IsDirectory;//是否为目录 0否 1是
	BOOL HasNext;//是否还有后续 0没有 1有
	char szFileName[256];//文件名

}FILEINFO, * PFILEINFO;

//查看具体原始数据
void Dump(BYTE* pData, size_t nSize);

class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	//组包
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
			TRACE("%s \r\n", strData.c_str() + 12);
			//memcpy(&strData[0], pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0;j < strData.size();j++) {
			sSum += BYTE(strData[j]) & 0xFF;//保留最后8位  BYTE（char）将char转换为ASCII码值  sum累加数据的ASCII码值
		}
		//TRACE("client packet: \r\n");
		//Dump((BYTE*)Data(), Size());
	}

	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
	}
	//解析包
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		for (;i < nSize;i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {//从接收的数据开始每次解析两个字节，直到找到包头
				sHead = *(WORD*)(pData + i);
				i += 2;//
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) { //包头后至少要有4字节长度，2字节命令，2字节和校验
			nSize = 0;
			return;//数据不够解析直接返回
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {//包未完全接收到
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i);  i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i);  i += 2;
		WORD sum = 0;
		//计算校验和
		for (size_t j = 0;j < strData.size();j++) {
			sum += BYTE(strData[j]) & 0xFF;//保留最后8位  BYTE（char）将char转换为ASCII码值  sum累加数据的ASCII码值
		}
		if (sum == sSum) {
			nSize = i;//包头2字节，长度4字节，命令2字节，数据nLength-4字节，和校验2字节
			return;
		}
		nSize = 0;//和校验失败，数据不合法
	}

	int Size() {
		return  nLength + 6;
	}

	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	~CPacket() {}

	CPacket& operator=(const CPacket& pack) {
		if (this == &pack) {
			return *this;
		}
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		return *this;
	}

	WORD sHead;//固定位 0xFEFF
	DWORD nLength;//包长度(从控制命令开始，到和校验结束)
	WORD sCmd;//控制命令
	std::string strData;//包数据
	WORD sSum;//和校验
	std::string strOut;//整个包数据
private:

};

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键、右键、中键
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSEEV;

//获取错误信息
std::string GetErrorInfo(int wsaErrCode);


class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		if (m_instance == nullptr) {//静态函数没有this指针
			m_instance = new CClientSocket();
		}
		return m_instance;
	}

 bool InitSocket(int nIP,int nPort) {
	    if (m_sock != INVALID_SOCKET) {
	   	 CloseSocket();
	    }
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1)  return false;
		//TODO:校验
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = htonl(nIP);
		serv_adr.sin_port = htons(nPort);
		if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("指定的IP地址不存在! ");
			return false;
		}
		int ret=connect(m_sock, (sockaddr*) &serv_adr, sizeof(serv_adr));
		if (ret == -1) {
			AfxMessageBox("连接失败!");
			//TRACE("连接失败: %d %s\r\n", WSAGetLastError(),GetErrorInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;


	}
	

	int  DealCommand() {
		//char buffer[1024];
		if (m_sock == -1) return -1;
		char* buffer = m_buffer.data();
		if (buffer == NULL) {
			TRACE("内存不足\r\n");
			return -2;
		}
		//memset(buffer, 0, BUFFER_SIZE);
		static size_t index = 0;
		while (true) {
		    int  len = recv(m_sock, buffer + index, (int)BUFFER_SIZE - index, 0);
			//TRACE("client recv len=%d\r\n", len);
			//Dump((BYTE*)buffer, len);
			if ((len <= 0) &&(index==0)) {
				return -1;
			}
			size_t Len = (size_t)len;
			index += Len;
			Len = index;
			m_packet = CPacket((BYTE*)buffer, Len);
			if (Len > 0) {
				memmove(buffer, buffer + Len, index - Len);
				index -= Len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool GetFilePath(std::string& strPath) {
		if (m_packet.sCmd >= 2 && (m_packet.sCmd <= 4)) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	int Send(const char* pData, int nSize) {
		if (m_sock == -1)   return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		//TRACE("m_sock:%d \r\n", m_sock);
		if (m_sock == -1)   return false;
		//std::string strSend;
		//strSend.append((char*)&pack.sHead, 2);
		//strSend.append((char*)&pack.nLength, 4);
		//strSend.append((char*)&pack.sCmd, 2);
		//strSend.append(pack.strData.c_str(), pack.strData.size());
		//strSend.append((char*)&pack.sSum, 2);
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

	CPacket& GetPacket() {
		return m_packet;
	}

	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

private:
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket(const CClientSocket& ss) {
		m_sock = ss.m_sock;
	}
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket() {
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}
	~CClientSocket() {
		closesocket(m_sock);
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
			CClientSocket* tmp = m_instance;
			m_instance = nullptr;
			delete tmp;
		}
	}

	static CClientSocket* m_instance;
	class Helper {
	public:
		Helper() {
			CClientSocket::getInstance();
		}
		~Helper() {
			CClientSocket::releaseinstance();
		}
	};
	static Helper m_helper;
};