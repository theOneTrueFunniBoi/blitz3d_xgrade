
#include "stdafx.h"
#include "prefs.h"
#include "libs.h"
#include "resource.h"
#include "blitzide.h"

#include <mmsystem.h>

//BlitzIDE blitzIDE;

char _credits[] = "\r\n";
char _credits2[] = ": Mark Sibly\r\n\r\n";
char _credist3[] = " SoLoud Edition: Mark Sibly\r\n\r\n";
char _credits4[] = ": funniman.exe (Oscar Hunt)\r\n\r\n"
	"Documentation: Mark Sibly, Simon Harrison, Paul Gerfen, Shane Monroe and the Blitz Doc Team\r\n\r\n"
	"Testing and support: James Boyd, Simon Armstrong and the Blitz Dev Team\r\n\r\n"
	"FreeImage Image loader courtesy of Floris van den berg\r\n\r\n";
class Dialog : public CDialog {
	bool _quit;
public:
	Dialog() : _quit(false) {}

	afx_msg void OnOK() {
		_quit = true;
	}

	void wait() {
		_quit = false;
		MSG msg;
		while (!_quit && GetMessage(&msg, 0, 0, 0)) {
			if (!AfxGetApp()->PreTranslateMessage(&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		EndDialog(0);
	}

	void wait(int n) {
		int _expire = (int) timeGetTime() + n;
		for (;;) {
			int tm = _expire - (int) timeGetTime();
			if (tm < 0) tm = 0;
			MsgWaitForMultipleObjects(0, 0, false, tm, QS_ALLEVENTS);

			MSG msg;
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				if (!AfxGetApp()->PreTranslateMessage(&msg)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			if (!tm) return;
		}
	}
};

void aboutBlitz(bool delay) {

	AfxGetMainWnd()->EnableWindow(0);

	Dialog about;

	about.Create(IDD_ABOUT);
	int bcc_ver=compiler_ver&0xffff;
	int ide_ver=VERSION&0xffff;
	int lnk_ver=linker_ver&0xffff;
	int run_ver=runtime_ver&0xffff;

	//may cause memory leaks, but delete buf segfaults and im too stupid to fix it
	char minor[5];
	char buf[5];

	itoa((bcc_ver) % 1000, minor, 5);
	sprintf(buf, "%03s", minor);

	//string bcc_v=itoa(bcc_ver/1000)+"."+itoa(bcc_ver%1000);
	string bcc_v = itoa(bcc_ver / 1000) + "." + buf;

	memset(buf, 0, sizeof buf);
	itoa((ide_ver) % 1000, minor, 5);
	sprintf(buf, "%03s", minor);

	//string ide_v=itoa(ide_ver/1000)+"."+itoa(ide_ver%1000);
	string ide_v = itoa(ide_ver / 1000) + "." + buf;

	memset(buf, 0, sizeof buf);
	itoa((lnk_ver) % 1000, minor, 5);
	sprintf(buf, "%03s", minor);

	//string lnk_v=itoa(lnk_ver/1000)+"."+itoa(lnk_ver%1000);
	string lnk_v = itoa(lnk_ver / 1000) + "." + buf;

	memset(buf, 0, sizeof buf);
	itoa((run_ver) % 1000, minor, 5);
	sprintf(buf, "%03s", minor);

	//string run_v=itoa(run_ver/1000)+"."+itoa(run_ver%1000);
	string run_v = itoa(run_ver / 1000) + "." + buf;

	// hack to get around shit
	string tmp = VersionConfig::blitzIdent;

	string credits = _credits;
	credits += VersionConfig::blitzIdentShortest;
	credits += _credits2;
	credits += VersionConfig::blitzIdentShortest;
	credits += _credist3;
	credits += VersionConfig::blitzIdent;
	credits += _credits4;

	if(runtime_ver>>16==3) {
		credits += "LibSGD (c) Mark Sibly";
		run_v+=" (LibSGD Build)";
	}else if(runtime_ver>>16 == 2) {
		credits += "SoLoud Audio engine (c) 2013-2018 Jari Komppa\r\n\r\n";
		run_v+=" (SoLoud Build)";
	}else if(runtime_ver>>16 == 1) {
		credits += "FMOD Audio engine (c) Firelight Technologies Pty Ltd\r\n\r\n";
		run_v+=" (FMOD Build)";
	}

	about.GetDlgItem(IDC_CREDITS)->SetWindowText(credits.c_str());

	string t= tmp+" IDE v"+ide_v;
	//string t = tmp + " v" + ide_v;
	about.GetDlgItem( IDC_PRODUCT )->SetWindowText( t.c_str() );

	//bit of cheating here, since i can't properly get the versions anymore i just use the ide version
	//t="Compiler v"+bcc_v +" Linker v"+lnk_v;
	t = "Compiler v" + ide_v + " Linker v" + ide_v;
	about.GetDlgItem(IDC_PRODUCT2)->SetWindowText(t.c_str());

	//t="Runtime v"+run_v;
	t = "Runtime v" + ide_v;
	about.GetDlgItem(IDC_VERSION)->SetWindowText(t.c_str());

	if (blitzIDE.SET_IMMERSIVE_DARK_MODE_SUCCESS)
	{
		DwmSetWindowAttribute(about.GetSafeHwnd(), DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE, &blitzIDE.SET_IMMERSIVE_DARK_MODE_SUCCESS, sizeof(blitzIDE.SET_IMMERSIVE_DARK_MODE_SUCCESS));
	}
	else if (blitzIDE.OLD_SET_IMMERSIVE_DARK_MODE_SUCCESS)
	{
		DwmSetWindowAttribute(about.GetSafeHwnd(), 19, &blitzIDE.OLD_SET_IMMERSIVE_DARK_MODE_SUCCESS, sizeof(blitzIDE.OLD_SET_IMMERSIVE_DARK_MODE_SUCCESS));
	}

	about.wait();
	about.EndDialog(0);
	AfxGetMainWnd()->EnableWindow(1);
}
