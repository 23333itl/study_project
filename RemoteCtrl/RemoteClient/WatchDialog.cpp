// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include"RemoteClientDlg.h"
#include "ClientSocket.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序

CPoint CWatchDialog::UserPoint2RemoteSceeenPoint( CPoint& point)
{
	//CPoint cur = point;
	CRect clientRect;
	ScreenToClient(&point);//屏幕坐标到客户区域坐标
	//本地坐标到远程坐标
	m_picture.GetWindowRect(&clientRect);
	int  width0 = clientRect.Width();
	int  height0 = clientRect.Height();
	int width = 1920, height = 1080;
	return CPoint(point.x * width / width0, point.y * height / height0);
}				  

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 60, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull()) {
			CRect rect;
			m_picture.GetWindowRect(&rect);
			//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(),0,0,SRCCOPY);
			pParent->GetImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);//缩放
			m_picture.InvalidateRect(NULL);
			pParent->GetImage().Destroy();
			pParent->SetImageStatus();
			//m_picture.ReleaseDC(pDC);
		}
	}
	CDialog::OnTimer(nIDEvent);
}

void CWatchDialog::OnStnClickedWatch()
{
	CPoint point;
	GetCursorPos(&point);
	//坐标转换
	CPoint remote = UserPoint2RemoteSceeenPoint(point);//将客户端（控制端）的坐标转换成服务端（被控端）的坐标
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nAction = 0;//左键
	event.nButton = 3;//双击
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
}


//鼠标按键事件
void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)//左键双击
{
	//坐标转换
	CPoint remote =UserPoint2RemoteSceeenPoint(point) ;//将客户端（控制端）的坐标转换成服务端（被控端）的坐标
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nAction = 0;//左键
	event.nButton = 2;//双击
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)//左键按下
{
	//坐标转换
	CPoint remote = UserPoint2RemoteSceeenPoint(point);//将客户端（控制端）的坐标转换成服务端（被控端）的坐标
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nAction = 0;//左键
	event.nButton = 3;//双击
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnLButtonDown(nFlags, point);
}

void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemoteSceeenPoint(point);//将客户端（控制端）的坐标转换成服务端（被控端）的坐标
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nAction = 0;//左键
	event.nButton = 4;//弹起
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnLButtonUp(nFlags, point);
}

void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)//右键双击
{
	//坐标转换
	CPoint remote = UserPoint2RemoteSceeenPoint(point);//将客户端（控制端）的坐标转换成服务端（被控端）的坐标
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nAction = 2;//右键
	event.nButton = 2;//双击
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnRButtonDblClk(nFlags, point);
}

void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemoteSceeenPoint(point);//将客户端（控制端）的坐标转换成服务端（被控端）的坐标
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nAction = 0;//左键
	event.nButton = 3;//按下//TODO:服务端需要做对应修改
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnRButtonDown(nFlags, point);
}

void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemoteSceeenPoint(point);//将客户端（控制端）的坐标转换成服务端（被控端）的坐标
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nAction = 0;//左键
	event.nButton = 4;//弹起
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnRButtonUp(nFlags, point);
}

void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	//坐标转换
	CPoint remote = UserPoint2RemoteSceeenPoint(point);//将客户端（控制端）的坐标转换成服务端（被控端）的坐标
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nAction = 0;//移动
	event.nButton = 2;//双击
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnMouseMove(nFlags, point);
}
