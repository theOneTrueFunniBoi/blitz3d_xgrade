
#pragma warning( disable:4786 )

#include "bbruntime_dll.h"
#include "../debugger/debugger.h"
//#include "../compiler/environ.h"
//#undef environ

using namespace std;

#include <map>
#include <eh.h>
#include <float.h>

#include "../bbruntime/bbruntime.h"

class DummyDebugger : public Debugger{
public:
	virtual void debugRun(){}
	virtual void debugStop(){}// bbruntime_panic(0); }
	virtual void debugStmt( int srcpos,const char *file ){}
	virtual void debugEnter( void *frame,void *env,const char *func ){}
	virtual void debugLeave(){}
	virtual void debugLog( const char *msg ){}
	virtual void debugMsg( const char *e,bool serious ){
		string tmpStr = "Blitz3D FATAL EXCEPTION - ";
		tmpStr+=e;
		tmpStr+="\n\r\n\rSCREENSHOT THIS ERROR, AND SEND IT TO THE APPLICATION DEVELOPER! Press OK to exit the application.";
		if( serious ) MessageBox( NULL,tmpStr.c_str(), "Blitz3D FATAL EXCEPTION!", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL | MB_TOPMOST | MB_SETFOREGROUND);
	}
	virtual void debugSys( void *msg ){}
};

static HINSTANCE hinst;
static map<const char*,void*> syms;
map<const char*,void*>::iterator sym_it;
static gxRuntime *gx_runtime;

static void rtSym( const char *sym,void *pc ){
	syms[sym]=pc;
}

static void _cdecl seTranslator( unsigned int u,EXCEPTION_POINTERS* pExp ){
	string panicStr = "Undefined Runtime Exception.";
	string panicDesc = "This error has no provided description as of yet! If you have a description for this error, contact FUNNIMAN.";
	bool clearDesc = false;
	switch( u ){
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		panicStr = "Integer divide by zero!";
		panicDesc = "Dividing integers by zero is an illegal operation.";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		panicStr = "Float divide by zero!";
		panicDesc = "Dividing floats by zero is an illegal operation.";
	case EXCEPTION_INT_OVERFLOW:
		panicStr = "Integer overflow!";
		panicDesc = "Integer exceeded it's absolute maximum value.";
	case EXCEPTION_ACCESS_VIOLATION:
		panicStr = "Unhandled Memory access violation!";
		panicDesc = "The application attempted to read or write to an invalid or protected memory address.";
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		panicStr = "Illegal instruction!";
		panicDesc = "The application attempted to execute an undefined instruction.";
	case EXCEPTION_STACK_OVERFLOW:
		panicStr = "Stack overflow!";
		panicDesc = "The application has exceeded it's stack memory limit.";
	case EXCEPTION_INVALID_HANDLE:
		panicStr = "Invalid handle!";
		panicDesc = "Attempted to use invalid kernel object. Perhaps it was closed?";
	case EXCEPTION_IN_PAGE_ERROR:
		panicStr = "In page error!";
		panicDesc = "Attempted to access invalid page, and the system failed to load the page.";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		panicStr = "Array bounds exceeded!";
		panicDesc = "Attempted to access out of bounds array element.";
	default:
		clearDesc = true;
	}

	// stupid hack to only not display the desc on an undefined exception while also removing the newline
	if (!clearDesc)
	{
		panicDesc = "\n" + panicDesc;
	}
	else {
		panicDesc = "";
	}

	panicStr = "FATAL: \n"+panicStr+panicDesc;
	bbruntime_panic( panicStr.c_str() );
}

int Runtime::version(){
	return VERSION;
}

const char *Runtime::nextSym(){
	if( !syms.size() ){
		bbruntime_link( rtSym );
		sym_it=syms.begin();
	}
	if( sym_it==syms.end() ){
		syms.clear();return 0;
	}
	return (sym_it++)->first;
}

int Runtime::symValue( const char *sym ){
	map<const char*,void*>::iterator it=syms.find( sym );
	if( it!=syms.end() ) return (int)it->second;
	return -1;
}

void Runtime::startup( HINSTANCE h ){
	hinst=h;
}

void Runtime::shutdown(){
	trackmem( false );
	syms.clear();
}

void Runtime::execute( void (*pc)(),const char *args,Debugger *dbg ){

	bool debug=!!dbg;

	static DummyDebugger dummydebug;

	if( !dbg ) dbg=&dummydebug;

	trackmem( true );

	_se_translator_function old_trans=_set_se_translator( seTranslator );
	_control87( _RC_NEAR|_PC_24|_EM_INVALID|_EM_ZERODIVIDE|_EM_OVERFLOW|_EM_UNDERFLOW|_EM_INEXACT|_EM_DENORMAL,0xfffff );

	//strip spaces from ends of args...
	string params=args;
	while( params.size() && params[0]==' ' ) params=params.substr( 1 );
	while( params.size() && params[params.size()-1]==' ' ) params=params.substr( 0,params.size()-1 );

	if( gx_runtime=gxRuntime::openRuntime( hinst,params,dbg ) ){

		bbruntime_run( gx_runtime,pc,debug );

		gxRuntime *t=gx_runtime;
		gx_runtime=0;
		gxRuntime::closeRuntime( t );
	}

	_control87( _CW_DEFAULT,0xfffff );
	_set_se_translator( old_trans );
}

void Runtime::asyncStop(){
	if( gx_runtime ) gx_runtime->asyncStop();
}

void Runtime::asyncRun(){
	if( gx_runtime ) gx_runtime->asyncRun();
}

void Runtime::asyncEnd(){
	if( gx_runtime ) gx_runtime->asyncEnd();
}

void Runtime::checkmem( streambuf *buf ){
	ostream out( buf );
	::checkmem( out );
}

Runtime *_cdecl runtimeGetRuntime(){
	static Runtime runtime;
	return &runtime;
}

/********************** BUTT UGLY DLL->EXE HOOK! *************************/

static void *module_pc;
static map<string,int> module_syms;
static map<string,int> runtime_syms;
static Runtime *runtime;

static void fail(){
	MessageBox( 0,"Unable to run Blitz Basic module",0,0 );
	ExitProcess(-1);
}

struct Sym{
	string name;
	int value;
};

static Sym getSym( void **p ){
	Sym sym;
	char *t=(char*)*p;
	while( char c=*t++ ) sym.name+=c;
	sym.value=*(int*)t+(int)module_pc;
	*p=t+4;return sym;
}

static int findSym( const string &t ){
	map<string,int>::iterator it;

	it=module_syms.find( t );
	if( it!=module_syms.end() ) return it->second;
	it=runtime_syms.find( t );
	if( it!=runtime_syms.end() ) return it->second;

	string err="Can't find symbol: "+t;
	MessageBox( 0,err.c_str(),0,0 );
	ExitProcess(0);
	return 0;
}

static void link(){

	while( const char *sc=runtime->nextSym() ){

		string t(sc);

		if( t[0]=='_' ){
			runtime_syms["_"+t]=runtime->symValue(sc);
			continue;
		}

		if( t[0]=='!' ) t=t.substr(1);

		if( !isalnum(t[0]) ) t=t.substr(1);

		for( int k=0;k<t.size();++k ){
			if( isalnum(t[k]) || t[k]=='_' ) continue;
			t=t.substr( 0,k );break;
		}

		runtime_syms["_f"+tolower(t)]=runtime->symValue(sc);
	}

	HRSRC hres=FindResource( 0,MAKEINTRESOURCE(1111),RT_RCDATA );if( !hres ) fail();
	HGLOBAL hglo=LoadResource( 0,hres );if( !hglo ) fail();
	void *p=LockResource( hglo );if( !p ) fail();

	int sz=*(int*)p;p=(int*)p+1;

	//replace malloc for service pack 2 Data Execution Prevention (DEP).
	module_pc=VirtualAlloc( 0,sz,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE );

	memcpy( module_pc,p,sz );
	p=(char*)p+sz;

	int k,cnt;

	cnt=*(int*)p;p=(int*)p+1;
	for( k=0;k<cnt;++k ){
		Sym sym=getSym( &p );
		if( sym.value<(int)module_pc || sym.value>=(int)module_pc+sz ) fail();
		module_syms[sym.name]=sym.value;
	}

	cnt=*(int*)p;p=(int*)p+1;
	for( k=0;k<cnt;++k ){
		Sym sym=getSym( &p );
		int *pp=(int*)sym.value;
		int dest=findSym( sym.name );
		*pp+=dest-(int)pp;
	}

	cnt=*(int*)p;p=(int*)p+1;
	for( k=0;k<cnt;++k ){
		Sym sym=getSym( &p );
		int *pp=(int*)sym.value;
		int dest=findSym( sym.name );
		*pp+=dest;
	}

	runtime_syms.clear();
	module_syms.clear();
}

extern "C" _declspec(dllexport) int _stdcall bbWinMain();
extern "C" BOOL _stdcall _DllMainCRTStartup( HANDLE,DWORD,LPVOID );

bool WINAPI DllMain( HANDLE module,DWORD reason,void *reserved ){
	return TRUE;
}

int __stdcall bbWinMain(){

	HINSTANCE inst=GetModuleHandle( 0 );

	_DllMainCRTStartup( inst,DLL_PROCESS_ATTACH,0 );

#ifdef BETA
	int ver=VERSION & 0x7fff;
	string t="Created with Blitz3D Beta V"+itoa( ver/100 )+"."+itoa( ver%100 );
	MessageBox( GetDesktopWindow(),t.c_str(),"Blitz3D Message",MB_OK );
#endif

#ifdef SCHOOLS
	MessageBox( GetDesktopWindow(),"Created with the schools version of Blitz Basic","Blitz Basic Message",MB_OK );
#endif

	runtime=runtimeGetRuntime();
	runtime->startup( inst );

	link();

	HMODULE dbgHandle = 0;
	Debugger* debugger = 0;

	//get cmd_line and params
	string cmd=GetCommandLine(),params;
	while( cmd.size() && cmd[0]==' ' ) cmd=cmd.substr( 1 );
	if( cmd.find( '\"' )==0 ){
		int n=cmd.find( '\"',1 );
		if( n!=string::npos ){
			params=cmd.substr( n+1 );
			cmd=cmd.substr( 1,n-1 );
		}
	}
	else if (!(cmd.find("/DEBUG")==std::string::npos)) {
		//acquire debugger
		dbgHandle = LoadLibrary("debugger.dll");
		if (!dbgHandle)
		{
			//perhaps it is in the parent dir?
			dbgHandle = LoadLibrary("../debugger.dll");
			if (!dbgHandle)
			{
				MessageBoxA(NULL, "Unable to acquire debugger", "Blitz Debugger Fatal Error", MB_SYSTEMMODAL|MB_ICONERROR|MB_TOPMOST);
				exit(-1);
			}
		}
		if (dbgHandle) {
			ifstream environSave;
			environSave.open("tmpapplication.environ", ios::out | ios::app | ios::binary);
			if (!environSave.is_open())
			{
				//perhaps it is in the parent dir?
				environSave.open("../tmpapplication.environ", ios::out | ios::app | ios::binary);
				if (!environSave.is_open())
				{
					MessageBoxA(NULL, "Unable to acquire code environment", "Blitz Debugger Fatal Error", MB_SYSTEMMODAL | MB_ICONERROR | MB_TOPMOST);
					exit(-1);
				}
			}

			streampos size = environSave.tellg();

			char* cEnviron = new char[size];

			environSave.seekg(0, ios::beg);
			//this is probably a really bad idea, but fuck it
			environSave.read((char*)environ, size);

			environSave.close();
			//typedef Debugger* (_cdecl* GetDebugger)(HMODULE,char*);
			//GetDebugger gd = (GetDebugger)GetProcAddress(dbgHandle, "debuggerGetDebugger");
			//if (gd) debugger = gd(inst, cEnviron);
			debugger = (Debugger*)GetProcAddress(dbgHandle, "debuggerGetDebugger");

			//delete[] environ;
		}

		if (!debugger) {
			MessageBoxA(NULL, "Error launching debugger", "Blitz Debugger Fatal Error", MB_SYSTEMMODAL | MB_ICONERROR | MB_TOPMOST);
			exit(-1);
		}
	} else {
		int n=cmd.find( ' ' );
		if( n!=string::npos ){
			params=cmd.substr( n+1 );
			cmd=cmd.substr( 0,n );
		}
	}

	if (!!debugger)
	{
		runtime->execute((void(*)())module_pc, params.c_str(), debugger);
	} else {
		runtime->execute((void(*)())module_pc, params.c_str(), 0);
	}
	runtime->shutdown();

	_DllMainCRTStartup( inst,DLL_PROCESS_DETACH,0 );

	ExitProcess(0);
	return 0;
}
