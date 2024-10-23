
//#include <string>
#include <iostream>
#include <windows.h>
#include "../versionconfig/versionconfig.h"
#include "../config/config.h"
#include <iomanip>

using namespace std;

// Avoiding the use of the <string> class reduces the size of the executable.
/*static string getAppDir(){
    char buff[MAX_PATH];
    if( GetModuleFileName( 0,buff,MAX_PATH ) ){
            string t=buff;
            int n=t.find_last_of( '\\' );
            if( n!=string::npos ) t=t.substr( 0,n );
            return t;
    }
    return "";
}*/

static int desktopDepth() {
    HDC hdc = GetDC(GetDesktopWindow());
    return GetDeviceCaps(hdc, BITSPIXEL);
}

static void fail(const char* p) {
    char bb_ret[255];
    strcpy(bb_ret, VersionConfig::blitzIdentShort);
    strcat(bb_ret, " Error");
    ::MessageBox(0, p, bb_ret, MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_TOPMOST | MB_ICONERROR);
    ExitProcess(-1);
}

static void getAppDir(char* buff, int buffSize) {
    if (GetModuleFileName(0, buff, buffSize)) {
        char* p = strrchr(buff, '\\');
        if (p != NULL) {
            *(p) = '\0'; // truncate the string at the last backslash
        }
    }
}

/*static void showInfo() {
    const int major = (VERSION & 0xffff) / 1000; 
    char minor[5];
    itoa((VERSION & 0xffff) % 1000, minor, 5);
    char buf[5];
    sprintf(buf, "%03s", minor);
    //may cause memory leaks, but delete buf segfaults and im too stupid to fix it
    char ret_msg[400];
    strcpy(ret_msg, VersionConfig::blitzIdentShort);
    strcat(ret_msg, " Launcher v" + major);
    strcat(ret_msg, ".");
    strcat(ret_msg, buf);
    strcat(ret_msg, "\n");
    OutputDebugString(ret_msg);
    OutputDebugString("Copyright 2000-2003 Blitz Research Ltd | Copyright 2024 funniman.exe\n");
}*/

static void showHelp() {
    fail("Usage: XGrade-Launcher.exe [-h|-mediaviewer|-noide] [sourcefile.bb]\n\n"
    "-h               : show this help\n\n"
    "-mediaviewer     : launch Blitz3D Media Viewer instead of the IDE\n\n"
    "-noide           : do not launch IDE (automatically invoked when using -h or -mediaviewer)");
}

static void syntaxError(const char* err = "")
{
    char ret_err[255];
    strcpy(ret_err, "Invalid Syntax: ");
    strcat(ret_err, err);
    strcat(ret_err, "\n\n\n\n"
    "Usage: XGrade-Launcher.exe [-h|-mediaviewer|-noide] [sourcefile.bb]\n\n"
    "-h               : show this help\n\n"
    "-mediaviewer     : launch Blitz3D Media Viewer instead of the IDE\n\n"
    "-noide           : do not launch IDE (automatically invoked when using -h or -mediaviewer)");
    fail(ret_err);
}

//int main(int argc, char* argv[])
//{
//    cout << VersionConfig::blitzIdentShort << " IDE Launcher" << endl;
//}
//
int _stdcall WinMain( HINSTANCE inst,HINSTANCE prev,char *cmd,int show )
{
    //cmd line stuff
    bool isMediaView=false,isHelp=false,nocont=false,silent=false;

    /*for (int k = 1; k<__argc; ++k)
    {
        const char* t = CharLowerA(__argv[k]);

        if (t == "-h") {
            isHelp=true;
        }
        else if (t == "-mediaviewer")
        {
            isMediaView=true;
        }
        else if (t == "-noide")
        {
            nocont=true;
        }
        //else if (t == "-s")
        //{
        //    silent=true;
        //}
        else {
            char arg_err[255];
            //strcpy(arg_err, "Unrecognized commandline flag: '");
            //strcat(arg_err, t);
            strcpy(arg_err, t);
            strcat(arg_err, "'");
            syntaxError(arg_err);
        }
    }*/

    //if (!silent) showInfo();

    if (isHelp) showHelp();

    if (desktopDepth() < 16)
    {
        //because we're not using string in the launcher to optimize, we gotta do this shit
        char md_err[255];
        strcpy(md_err, "Your desktop must be in high-colour mode to use ");
        strcat(md_err, VersionConfig::blitzIdentShort);
        strcat(md_err, ".\n\nYou can change your display settings from the control panel.");

        fail(md_err);
    }

    //Ugly hack to get application dir...
    /*string t = getAppDir();
    putenv( ("blitzpath="+t).c_str() );
    SetCurrentDirectory( t.c_str() );
    t=t+"\\bin\\ide.exe "+cmd;*/

    char envVar[MAX_PATH];
    strcpy(envVar, "blitzpath=");

    char t[MAX_PATH];
    getAppDir(t, MAX_PATH);

    strcat(envVar, t);
    putenv(envVar);
    SetCurrentDirectory(t);

    if (!isMediaView)
    {
        strcat(t, "\\bin\\ide.exe ");
        for ( int k=1;k<__argc;++k )
        {
            strcat(t, __argv[k]);
            strcat(t, " ");
        }
    }
    else
    {
        strcat(t, "\\bin\\blitzviewer.exe ");
    }

    if (nocont) return 0;

    //cout << "Starting, please wait..." << endl;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si,sizeof(si));si.cb=sizeof(si);
	if( !CreateProcess( 0,t,0,0,0,0,0,0,&si,&pi ) ){
        char bb_ret2[255];
        strcpy(bb_ret2, VersionConfig::blitzIdentShort);
        strcat(bb_ret2, " Error");
        char bb_err[255];
        strcpy(bb_err, "Unable to run ");
        strcat(bb_err, VersionConfig::blitzIdentShort);
        strcat(bb_err, ".");
		::MessageBox( 0,bb_err,bb_ret2,MB_SYSTEMMODAL|MB_SETFOREGROUND|MB_TOPMOST|MB_ICONERROR );
		ExitProcess(-1);
	}

	//wait for BB to start
	WaitForInputIdle( pi.hProcess,INFINITE );

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    return 0;
}
