
#include "stdafx.h"
#include "debugger.h"
#include "debuggerapp.h"
#include "resource.h"
#include "../blitzide/prefs.h"
#include "../versionconfig/versionconfig.h"

DebuggerApp debuggerApp;

DebuggerApp::~DebuggerApp(){
}

BOOL DebuggerApp::InitInstance(){

	AfxInitRichEdit();

	main_frame=new MainFrame();

	m_pMainWnd=main_frame;

	RECT rect;
	SystemParametersInfo( SPI_GETWORKAREA,0,&rect,0 );

	int x=rect.left;
	int w=rect.right-x;
	int h=240;
	int y=rect.bottom-h;

	// hack to get around shit
	string tmp = VersionConfig::blitzIdent;

	main_frame->Create( 0,(tmp+" Debugger").c_str(),
		WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
		CRect( x,y,x+w,y+h ) );
	main_frame->ShowWindow( SW_SHOW );
	main_frame->UpdateWindow();

	return TRUE;
}

int DebuggerApp::ExitInstance(){
	main_frame->DestroyWindow();
	return 0;
}

MainFrame *DebuggerApp::mainFrame(){
	return debuggerApp.main_frame;
}

Debugger * _cdecl debuggerGetDebugger( void *mod,void *env ){
	debuggerApp.mainFrame()->setRuntime( mod,env );
	return debuggerApp.mainFrame();
}
