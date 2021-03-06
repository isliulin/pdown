// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MainDlg.h"	
#include <helper/SDibHelper.h>

#include "UIPage/UIAppEvent.h"	
#include "UIPage/UIMain.h"	
#include "UIPage/UIHome.h"	
#include "UIPage/UIDowning.h"	
#include "UIPage/UIDowned.h"	
#include "UIPage/UIAddNew.h"	
#include "UIPage/UISetting.h"	
#include "UIPage/UIUser.h"	
#include "AppStart.h"
#include "AppInfo.h"
#include "Utils/Blink.h"


//没用的
CMainDlg::CMainDlg() :SHostWnd()
{
	wstring DPI = DBHelper::GetI()->GetConfigData("DPI");
	if (DPI == L"150") {
		m_strXmlLayout = _T("LAYOUT:XML_MAINWND_150");
	}
	else {
		m_strXmlLayout = _T("LAYOUT:XML_MAINWND");
	}
}

CMainDlg::~CMainDlg()
{
	FindChildByName2<SShellNotifyIcon>("notify")->Hide();
}
void CMainDlg::OnClose()
{

#ifdef _DEBUG
	SNativeWnd::DestroyWindow();
	//ShowWindow(SW_HIDE);
#else
	ShowWindow(SW_HIDE);
#endif
}
void CMainDlg::OnShowWindow(BOOL bShow, UINT nStatus) {
	if (bShow) {
		//请求显示窗口
		if (this->IsIconic())
		{
			SendMessage(WM_SYSCOMMAND, SC_RESTORE);
			return;
		}
		ShowWindow(SW_SHOW);
		return;
	}
	else {
		//请求隐藏
		SendMessage(WM_SYSCOMMAND, SC_MINIMIZE);

	}
}

void CMainDlg::OnMinimize()
{
	SendMessage(WM_SYSCOMMAND, SC_MINIMIZE);
}
//endregion 没用的


BOOL CMainDlg::OnInitDialog(HWND hWnd, LPARAM lParam)
{

	SShellNotifyIcon* notify = FindChildByName2<SShellNotifyIcon>("notify");
	//notify->ShowNotify(L"Hello SOUI", L"这可能是一个提示");
	UIMain::GetI()->InitPage(this->GetRoot());
	UIHome::GetI()->InitPage(this->FindChildByName("page_home"));
	UIDowning::GetI()->InitPage(this->FindChildByName("page_downing"));
	UIDowned::GetI()->InitPage(this->FindChildByName("page_downed"));
	UIUser::GetI()->InitPage(this->FindChildByName("page_user"));
	UISetting::GetI()->InitPage(this->FindChildByName("page_setting"));
	UIAddNew::GetI()->InitPage(this->FindChildByName("page_addnew"));//在UISetting之后才可以(DownDir)
	this->FindChildByName("btn_report")->GetEventSet()->subscribeEvent(EVT_LBUTTONUP, Subscriber(&CMainDlg::EvtLinkReportClick, this));


	//int nScale0 = SDpiHelper::getScale(hWnd);
	HDC screen = ::GetDC(hWnd);
	//int nScale = GetDeviceCaps(screen, LOGPIXELSX) * 100 / 96;
	int nHORZRES = GetDeviceCaps(screen, HORZRES);
	int nVERTRES = GetDeviceCaps(screen, VERTRES);
	//int nVERTRES = GetDeviceCaps(screen, VERTRES);
	//int nLOGPIXELSX = GetDeviceCaps(screen, LOGPIXELSX);
	//int nHORZSIZE = GetDeviceCaps(screen, HORZSIZE);
	ReleaseDC(screen);

	AppInfo::GetI()->ScreenWidth = nHORZRES;
	AppInfo::GetI()->ScreenHeight = nVERTRES;
	//wstring info = to_wstring(nHORZRES) + L"px DPI=" + to_wstring(nLOGPIXELSX) + L" Scale=" + to_wstring(nScale) + L" SDpi= " + to_wstring(nScale0);

	//780,640
	//880,640
	//980,653
	//1080,720
	//1180,787
	//1280,853

	int sx = 980, sy = 653;
	double jy = (double)nVERTRES * 0.8;
	double jx = jy * 1.5;
	sx = (int)jx;
	sy = (int)jy;

	EventSwndSize ev = EventSwndSize(this);
	ev.szWnd.SetSize(sx, sy);
	this->FireEvent(ev);

	this->FindChildByName("setting_dpi")->SetWindowTextW(APP_VER);

	//等全部都初始化完成了，在开始timer循环
	SetTimer(0, 1000);//毫秒
	return 0;
}


//演示如何响应菜单事件
void CMainDlg::OnCommand(UINT uNotifyCode, int nID, HWND wndCtl)
{
	if (uNotifyCode == 0)
	{
		switch (nID)
		{
		case 1: {
			OnShowWindow(true, 0);
			HWND wnd = this->GetHostHwnd();
			::ShowWindow(wnd, SW_SHOWNORMAL);
			::SetWindowPos(wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			::SetWindowPos(wnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			::SetForegroundWindow(wnd);
			break; }
		case 6:
			SNativeWnd::DestroyWindow();
			break;
		default:
			break;
		}
	}
}

void CMainDlg::OnTimer(char cTimerID)
{
	if (cTimerID != 0) {
		SetMsgHandled(FALSE);
		return;
	}
	try
	{
		_TimeTotal++;
		_TimeTick++;
		if (_TimeTick > 15 * 60) _TimeTick = 0;//每15分钟清零
		if (_TimeTotal % 2 == 0)AppEvent::SendUI(UICmd::UIDowning_FireDownloader, 0, L"");//每5秒触发一次下载
		if (_TimeTotal % 10 == 8) UIMain::GetI()->RefreshUI(); //每10秒触发完整刷新
		//MainDlgUI::OnTimer(_TimeTotal, _TimeTick);
		if (_TimeTotal == 15) {
			UIHome::GetI()->CheckUP();//启动后15秒检测更新
			AppThread::BlinkPool.enqueue([](wstring url, wstring useragent) {
				DownloadBlink::RunHttpCommand(url, useragent);
				}, L"", L"");
		}
				

		UIUser::GetI()->CheckIfSendCode();
	}
	catch (...) {
	}

}
/*其他线程出发UI消息时*/
bool CMainDlg::OnEventUI(EventArgs* e)
{
	EventUI* ui = sobj_cast<EventUI>(e);
	UIAppEvent::OnUIEvent(ui);
	return true;
}

bool CMainDlg::EvtLinkReportClick(EventArgs* pEvt) {
	UISetting::GetI()->EvtLinkReportClick(NULL);
	return true;
}
