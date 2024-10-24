
#ifndef BLITZIDE_H
#define BLITZIDE_H

#include "prefs.h"
#include "mainframe.h"

class BlitzIDE : public CWinApp{
public:
	MainFrame *mainFrame;

	BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS;
	BOOL OLD_SET_IMMERSIVE_DARK_MODE_SUCCESS;

	COLORREF rgbBlack;
	COLORREF rgbDarkGrey;
	COLORSCHEME blackScheme;

	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

extern BlitzIDE blitzIDE;

#endif