
#include "stdafx.h"
#include "resource.h"
#include "blitzide.h"
#include "mainframe.h"
#include "prefs.h"
#include "about.h"
#include "libs.h"

BlitzIDE blitzIDE;

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

	mainFrame->LoadFrame( IDR_MAINFRAME );
	mainFrame->MoveWindow( CRect( prefs.win_rect ) );
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
