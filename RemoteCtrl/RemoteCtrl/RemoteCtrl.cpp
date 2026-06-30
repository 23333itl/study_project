// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include"ServerSocket.h"
#include"LockDialog.h"
#include<direct.h>
#include<stdio.h>
#include<io.h>
#include<list>
#include<atlimage.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
//查看具体原始数据
void Dump(BYTE* pData, size_t nSize) {
    std::string strOUT;
    for (size_t i = 0;i < nSize;i++) {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOUT += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i]&0xFF);
		strOUT += buf;
    }
    strOUT += "\n";
	OutputDebugStringA(strOUT.c_str());
}

int  MakeDriverInfo() {//1==>A 2==>B 3==>C ...26==>Z
    std::string result;
	for (int i = 1;i <= 26;i++) {
        if (_chdrive(i) == 0) {
            if(result.size()>0){
                result += ',';
            }
            result += 'A' + i - 1;
        }
	}
	CPacket pack(1, (BYTE*)result.c_str(), result.size());
    //Dump((BYTE*)pack.Data(), pack.Size());
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int MakeDiretoryInfo() {
    std::string strPath;
    //std::list<file_info> lstFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前命令不是获取文件列表，命令解析错误！！"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {//尝试把程序当前工作目录切换到strPath  切换失败
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        //lstFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限访问目录！！"));
        return -2;
    }
    _finddata_t fdata;
    intptr_t  hfind = 0;
    //int hfind = 0;
    if ((hfind=_findfirst("*", &fdata)) == -1) {//查找当前目录下所有文件 / 文件夹
        OutputDebugString(_T("没有找到任何文件！！"));
        FILEINFO finfo;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        return -3;
    }
    int Count = 0;
    do {
        FILEINFO finfo;
        //finfo.IsInvalid = FALSE;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        //strncpy_s(finfo.szFileName, sizeof(finfo.szFileName), fdata.name, _TRUNCATE);
        TRACE("%s \r\n", finfo.szFileName);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        Count++;
    } while (!_findnext(hfind, &fdata));
    TRACE("Server Count: %d \r\n", Count);
    //发送信息到控制端
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::getInstance()->Send(pack);
    _findclose(hfind);
    return 0;
}

int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(),NULL,NULL,SW_SHOWNORMAL);
    CPacket pack(2, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}
#pragma warning(disable:4966) //fopen sprintf strcpy strstr
int DownloadFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    FILE* pFile = NULL;
    errno_t err=fopen_s(&pFile,strPath.c_str(), "rb"); //rb 以二进制方式读
    //FILE* pFile = fopen(strPath.c_str(), "rb");
    long long data = 0;
    if (err!=0) {
        CPacket pack(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile);
        CPacket head(4, (BYTE*)&data, 8);//head 记录文件长度
        CServerSocket::getInstance()->Send(head);
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024];
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024);
        fclose(pFile);
    }
    CPacket pack(4, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int DeleteLocalFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    TCHAR sPath[MAX_PATH] = _T("");
    //mbstowcs(sPath, strPath.c_str(), strPath.size());//中文容易乱码
	MultiByteToWideChar(CP_ACP, 0, strPath.c_str(),strPath.size(), sPath,sizeof(sPath)/sizeof(TCHAR));
    //DeleteFile(sPath);
    DeleteFileA(strPath.c_str());
    CPacket pack(9, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret= %d\r\n", ret);
    return 0;
}

int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
        DWORD nFlags = 0;
        switch (mouse.nButton) {
        case 0://左键
            nFlags = 1;
            break;
        case 1://右键
            nFlags = 2;
            break;
        case 2://中键
            nFlags = 4;
            break;
        case 4:
            nFlags = 8;//没有按键
            break;
        }
        if (nFlags != 8)  SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        switch (mouse.nAction) {
        case 0://单击
            nFlags |= 0x10;
            break;
        case 1://双击
            nFlags |= 0x20;
            break;
        case 2://按下
            nFlags |= 0x40;
            break;
        case 3://放开
            nFlags |= 0x80;
            break;
        default:
            break;
         
        }
        TRACE("mouse event: %08X x: %d y: %d\r\n", nFlags,mouse.ptXY.x,mouse.ptXY.y);
        switch (nFlags) {
        case 0x21://左键双击  模拟两次单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());          
        case 0x11://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81://左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12://右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42://右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
		case 0x82://右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24://中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14://中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN,0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44://中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84://中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
		case 0x08://单纯鼠标移动
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
        }
        CPacket pack(5, NULL, 0);
        CServerSocket::getInstance()->Send(pack);

        
    }
    else {
        OutputDebugString(_T("获取鼠标操作参数失败！"));
        return -1;
    }

    return 0;
}

int SendScreen() {
    CImage screen;//GDI
    HDC hScreen =::GetDC(NULL);//设备上下文
	int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//
	int nWidth = GetDeviceCaps(hScreen,HORZRES);
	int nHeight = GetDeviceCaps(hScreen, VERTRES);//获取屏幕宽高
    screen.Create(nWidth, nHeight, nBitPerPixel);
	BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
    //BitBlt(screen.GetDC(), 0, 0, 978, 586, hScreen, 0, 0, SRCCOPY);
	ReleaseDC(NULL, hScreen);
    //for (int i = 0;i < 10;i++) {
    //    DWORD tick = GetTickCount64();
    //    screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
    //    TRACE("png %d\r\n", GetTickCount64() - tick);
    //    tick = GetTickCount64();
    //    screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
    //    TRACE("jpg %d\r\n", GetTickCount64() - tick);
    //}
    //screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
    HGLOBAL hMem=GlobalAlloc(GMEM_MOVEABLE,0);
    if (hMem == NULL) return -1;
    IStream* pStream = NULL;
	HRESULT ret=CreateStreamOnHGlobal(hMem, TRUE, &pStream);//sresult=0成功 其他失败
    if (ret == S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);
        PBYTE pData = (PBYTE)GlobalLock(hMem);
        SIZE_T nSize = GlobalSize(hMem);
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);
    }
    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();
    return 0;
}


CLockDialog dlg;
unsigned threadid = 0;

unsigned __stdcall threadLockDlg(void* arg) {
	TRACE("%s (%d)%d \r\n", __FUNCTION__, __LINE__,GetCurrentThreadId());
    dlg.Create(IDD_DIALOG_INFO, NULL);
    dlg.ShowWindow(SW_SHOW);
    //遮蔽后台窗口
    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
    rect.bottom = (LONG)GetSystemMetrics(SM_CYFULLSCREEN) * 1.2;
    //rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
    TRACE("screen right:%d bottom:%d\r\n", rect.right, rect.bottom);
    dlg.MoveWindow(rect);
    CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
    //将文本居中
    if (pText) {
        CRect rtText;
        pText->GetWindowRect(rtText);
        int nWidth = rtText.Width();
        int x = (rect.right - nWidth) / 2;
        int nHeight = rtText.Height();
        int y = (rect.bottom - nHeight) / 2;
        pText->MoveWindow(x,y,rtText.Width(),rtText.Height());
    }
    //窗口置顶
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);//永远在最前面，无法被覆盖 → 锁屏效果
    ShowCursor(false);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);//隐藏任务栏，开始菜单
    //限制鼠标移动范围和功能
    dlg.GetWindowRect(rect);
    ClipCursor(rect);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN) {
            TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
            if (msg.wParam == 0x1B) {//    按下 a键  ESC退出
                break;
            }
        }
    }
    //恢复鼠标
    ShowCursor(true);
    ClipCursor(NULL);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);//恢复任务栏，开始菜单
    
    dlg.DestroyWindow();
    _endthreadex(0);
    return 0;
}

int LockMachine() {
	if ((dlg.m_hWnd == NULL)||(dlg.m_hWnd==INVALID_HANDLE_VALUE) ) {
       // _beginthread(threadLockDlg, 0, NULL);
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid= %d \r\n", threadid);
	}
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnLockMachine() {
	//dlg.SendMessage(WM_KEYDOWN, 0x1B,0x00010001);
    //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x1B, 0x00010001);
    PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0);
    CPacket pack(8, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int TestConnect() {
	CPacket pack(1981, NULL, 0);
    bool ret=CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret= %d\r\n", ret);
    return 0;
}

int ExcuteCommand(int nCmd) {
    int ret = 0;
    switch (nCmd) {
    case 1://查看磁盘分区
        ret=MakeDriverInfo();
        break;
    case 2://查看指定目录下的文件
        ret = MakeDiretoryInfo();
        break;
    case 3://打开文件
        ret = RunFile();
        break;
    case 4://下载文件
        ret = DownloadFile();
        break;
    case 5:
        ret = MouseEvent();
        break;
    case 6://发送屏幕内容==>发送屏幕截图
        ret = SendScreen();
        break;
    case 7://锁机
        ret = LockMachine();
        //Sleep(50);
        //LockMachine();
        break;
    case 8://解锁
        ret = UnLockMachine();
        break;
    case 9://删除文件
        ret = DeleteLocalFile();
        break;
    case 1981:
        TestConnect();
        break;
    }
    return ret;
    //while (dlg.m_hWnd != NULL) {
    //    Sleep(10);
    //}
}


int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
    //        //server;
    //        // TODO: 在此处为应用程序的行为编写代码。
            CServerSocket* pserver = CServerSocket::getInstance();
            int count = 0;
            if (pserver->InitSocket() == false) {
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态!"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            while (CServerSocket::getInstance() != nullptr) {            
                if (pserver->AcceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                    count++;
                }
                //TRACE("AcceptClient return true\r\n");
				int ret=pserver->DealCommand();
				//TRACE("DealCommand ret %d \r\n", ret);
                if (ret > 0) {
                    ret=ExcuteCommand(pserver->GetPacket().sCmd);
                    if (ret != 0) {
                        TRACE("执行命令%d失败！ret=%d \r\n", pserver->GetPacket().sCmd, ret);
                    }
                    pserver->CloseClient();
                    TRACE("command is done");
                }

            }
           
			//while ((dlg.m_hWnd != NULL )&& (dlg.m_hWnd != INVALID_HANDLE_VALUE) ) {
			//	Sleep(100);

			//}
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
