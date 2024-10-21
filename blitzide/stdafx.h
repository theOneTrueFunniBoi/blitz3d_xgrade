
#ifndef STDAFX_H
#define STDAFX_H

#define _WIN32_WINNT 0x601

#pragma warning(disable:4786)

#include <afxwin.h>         // Core
#include <afxrich.h>		// CRich edit
#include <afxhtml.h>		// CHtmlView

#include "../stdutil/stdutil.h"

#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include "../versionconfig/versionconfig.h"

using namespace std;

/*class COvrHtmlEditCtrl : public CHtmlEditCtrl
{
public:
	virtual void _OnNavigateComplete2(LPDISPATCH, VARIANT FAR*)
	{
		//SetDesignMode(TRUE);
	}

	void _OnBeforeNavigate2(LPDISPATCH, VARIANT* URL, VARIANT*, VARIANT*, VARIANT*, VARIANT*, VARIANT_BOOL*)
	{
		CString str(V_BSTR(URL));
		int pos = str.Find(L'#');
		if (pos >= 0)
		{
			str = str.Mid(pos + 1);
			ShellExecute(NULL, "open", str, NULL, NULL, SW_SHOWNORMAL);
		}
	}

	DECLARE_EVENTSINK_MAP()
};

BEGIN_EVENTSINK_MAP(COvrHtmlEditCtrl, CHtmlEditCtrl)
	ON_EVENT_REFLECT(COvrHtmlEditCtrl, 250, _OnBeforeNavigate2, VTS_DISPATCH VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PBOOL)
	ON_EVENT_REFLECT(COvrHtmlEditCtrl, 252, _OnNavigateComplete2, VTS_DISPATCH VTS_PVARIANT)
END_EVENTSINK_MAP()*/

#endif