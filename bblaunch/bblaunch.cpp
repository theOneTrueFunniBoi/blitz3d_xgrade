
//#include <string>
#include <windows.h>
#include "../versionconfig/versionconfig.h"

//using namespace std;

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

static void getAppDir(char* buff, int buffSize) {
    if (GetModuleFileName(0, buff, buffSize)) {
        char* p = strrchr(buff, '\\');
        if (p != NULL) {
            *(p) = '\0'; // truncate the string at the last backslash
        }
    }
}

static void fail( const char *p ){
    char bb_ret[255];
    strcpy(bb_ret, VersionConfig::blitzIdentShort);
    strcat(bb_ret, " Error");
    ::MessageBox( 0,p, bb_ret, MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_TOPMOST | MB_ICONERROR);
    ExitProcess(-1);
}

static int desktopDepth(){
    HDC hdc=GetDC( GetDesktopWindow() );
    return GetDeviceCaps( hdc,BITSPIXEL );
}

int _stdcall WinMain( HINSTANCE inst,HINSTANCE prev,char *cmd,int show ){

    //because we're not using string in the launcher to optimize, we gotta do this shit
    char bb_err[255];
    strcpy(bb_err, "Unable to run ");
    strcat(bb_err, VersionConfig::blitzIdentShort);
    strcat(bb_err, ".");

    char md_err[255];
    strcpy(md_err, "Your desktop must be in high-colour mode to use ");
    strcat(md_err, VersionConfig::blitzIdentShort);
    strcat(md_err, ".\n\nYou can change your display settings from the control panel.");

    if( desktopDepth()<16 ) fail( md_err );

    //Ugly hack to get application dir...
    /*string t = getAppDir();
    putenv( ("blitzpath="+t).c_str() );
    SetCurrentDirectory( t.c_str() );
    t=t+"\\bin\\ide.exe "+cmd;*/

    char t[MAX_PATH];
    getAppDir(t, MAX_PATH);

    char envVar[MAX_PATH];
    strcpy(envVar, "blitzpath=");
    strcat(envVar, t);
    putenv(envVar);
    SetCurrentDirectory(t);

    strcat(t, "\\bin\\ide.exe ");
    strcat(t, cmd);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si,sizeof(si));si.cb=sizeof(si);
	if( !CreateProcess( 0,t,0,0,0,0,0,0,&si,&pi ) ){
        char bb_ret2[255];
        strcpy(bb_ret2, VersionConfig::blitzIdentShort);
        strcat(bb_ret2, " Error");
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
