
#include "stdafx.h"
#include "resource.h"
#include "blitzide.h"
#include "mainframe.h"
#include "prefs.h"
#include "about.h"
#include "libs.h"
#include <dwmapi.h>
#include <winrt/windows.ui.viewmanagement.h>

BlitzIDE blitzIDE;

//stuff to determine whether dark mode is supported
typedef LONG NTSTATUS, * PNTSTATUS;
#define STATUS_SUCCESS (0x00000000)  

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

RTL_OSVERSIONINFOW GetRealOSVersion() {
	HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
	if (hMod) {
		RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
		if (fxPtr != nullptr) {
			RTL_OSVERSIONINFOW rovi = { 0 };
			rovi.dwOSVersionInfoSize = sizeof(rovi);
			if (STATUS_SUCCESS == fxPtr(&rovi)) {
				return rovi;
			}
	}
}
	RTL_OSVERSIONINFOW rovi = { 0 };
	return rovi;
}

BOOL BlitzIDE::InitInstance(){

#ifdef _DEBUG
	AfxEnableMemoryTracking( true );
#endif

	AfxInitRichEdit();

	prefs.open();

	initLibs();

	mainFrame=new MainFrame();
	m_pMainWnd = mainFrame;

	HICON icn = LoadIcon(MAKEINTRESOURCE(IDI_ICON1));

	BOOL USE_DARK_MODE = true;

	//DwmSetWindowAttribute(mainFrame->GetSafeHwnd(), DWMWINDOWATTRIBUTE::DWMWA_CAPTION_COLOR, (void*)true, sizeof(true));
	//DwmSetWindowAttribute(mainFrame->GetSafeHwnd(), DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE, (void*)USE_DARK_MODE, sizeof(USE_DARK_MODE));

	RTL_OSVERSIONINFOW winNum = GetRealOSVersion();

	//AfxMessageBox(winNum.dwMajorVersion, MB_ICONWARNING | MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_TOPMOST);

	mainFrame->LoadFrame( IDR_MAINFRAME );
	mainFrame->MoveWindow( CRect( prefs.win_rect ) );
	//are we windows 10 or up? if so, enable dark mode
	if (winNum.dwMajorVersion > 6)
	{
		winrt::Windows::UI::ViewManagement::UISettings settings;
		auto background = settings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Background);
		auto foreground = settings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Foreground);
		if (background.A == 255 && background.R == 0 && background.G == 0 && background.B == 0)
		{
			SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(mainFrame->GetSafeHwnd(), DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE, &USE_DARK_MODE, sizeof(USE_DARK_MODE)));
			if (!SET_IMMERSIVE_DARK_MODE_SUCCESS) OLD_SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(mainFrame->GetSafeHwnd(), 19, &USE_DARK_MODE, sizeof(USE_DARK_MODE)));
			if (!OLD_SET_IMMERSIVE_DARK_MODE_SUCCESS && !SET_IMMERSIVE_DARK_MODE_SUCCESS)
			{
				string exitMsg = VersionConfig::blitzIdentShort;
				exitMsg += " was unable to set the reactive window bar flag! The application will continue in light mode...";
				AfxMessageBox(exitMsg.c_str(), MB_ICONWARNING | MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_TOPMOST);
			}
		}
	}
	mainFrame->ShowWindow( m_nCmdShow );
	mainFrame->UpdateWindow();
	mainFrame->SetIcon(icn, true);

	//mainFrame->SetTaskbarOverlayIcon(icn, "Application Icon");

	if( prefs.win_maximized ) mainFrame->ShowWindow( SW_SHOWMAXIMIZED );

	return TRUE;
}

int BlitzIDE::ExitInstance(){

	prefs.close();

	return 0;
}
