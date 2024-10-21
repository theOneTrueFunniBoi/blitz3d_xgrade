
#pragma warning(disable:4786)

#include "libs.h"

#include <iostream>

#include "../config/config.h"
#include "../stdutil/stdutil.h"

#include <set>
#include <map>
#include <list>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;

#include "../linker/linker.h"
#include "../compiler/environ.h"
#include "../compiler/parser.h"
#include "../compiler/assem_x86/assem_x86.h"
#include "../compiler/codegen_x86/codegen_x86.h"
#include "../bbruntime_dll/bbruntime_dll.h"

#undef environ

static void showInfo(){
	const int major=(VERSION&0xffff)/1000,minor=(VERSION&0xffff)%1000;
	cout<<"BlitzCC MAV-Less v"<<major<<"."<<minor<<endl;
	cout<<"(C)opyright 2000-2003 Blitz Research Ltd"<<endl;
}

static void showUsage(){
	cout<<"Usage: blitzcc [-h|-q|+q|-c|-d|-k|+k|-v|-hook|-blitzpath path|-noinputerr|-ide|-o exefile] [sourcefile.bb]"<<endl;
}

static void showHelp(){
	showUsage();
	cout<<"-h               : show this help"<<endl;
	cout<<"-q               : quiet mode"<<endl;
	cout<<"+q               : very quiet mode"<<endl;
	cout<<"-c               : compile only"<<endl;
	cout<<"-d               : debug compile"<<endl;
	cout<<"-k               : dump keywords"<<endl;
	cout<<"+k               : dump keywords and syntax"<<endl;
	cout<<"-v               : version info"<<endl;
	cout<<"-hook            : use runtime hook instead of generating temp executable"<<endl;
	cout<<"-blitzpath path  : manually specify the path to your blitz3d install"<<endl;
	cout<<"-noinputerr      : suppresses certain errors relating to the input file"<<endl;
	cout<<"-ide             : treats all commands as though they are run by the ide"<<endl;
	cout<<"-o exefile       : generate executable"<<endl;
	exit(0);
}

static void err( const string &t ){
	cout<<t<<endl;
	exit(-1);
}

static string quickHelp( const string &kw ){

	Environ *e=runtimeEnviron;
	Decl *d=e->funcDecls->findDecl( tolower( kw ) );
	if( !d || d->type->funcType()==0 ) return "No quick help available for "+kw;
	string t=kw;
	FuncType *f=d->type->funcType();
	if( f->returnType==Type::float_type ) t+='#';
	else if( f->returnType==Type::string_type ) t+='$';

	t+=" ";

	if( f->returnType!=Type::void_type ) t+="( ";

	for( int k=0;k<f->params->size();++k ){
		string s;
		if( k ) s+=',';
		Decl *p=f->params->decls[k];s+=p->name;
		if( p->type==Type::float_type ) s+='#';
		else if( p->type==Type::string_type ) s+='$';
		else if( p->type==Type::void_type ) s+='*';
		if( p->defType ) s='['+s+']';
		t+=s;
	}

	if( f->returnType!=Type::void_type ){
		t+=f->params->size() ? " )" : ")";
	}
	return t;
}

static void dumpKeys( bool lang,bool mod,bool help ){

	if( lang ){
		map<string,int>::iterator it;
		map<string,int> &keywords=Toker::getKeywords();
		for( it=keywords.begin();it!=keywords.end();++it ){
			if( it->first.find(' ')!=string::npos ) continue;
			cout<<it->first<<endl;
		}
	}

	if( !mod ) return;

	for( int k=0;k<keyWords.size();++k ){
		string t=keyWords[k];

		if( t[0]=='_' )	continue;
		if( !isalpha( t[0] ) ) t=t.substr( 1 );
		for( int n=0;n<t.size();++n ){
			if( !isalnum(t[n]) && t[n]!='_' ){
				t=t.substr( 0,n );
				break;
			}
		}
		if( help ) t=quickHelp(t);
		cout<<t<<endl;
	}
}

static string verstr( int ver ){
	string b;
	if(ver>>16==3) {
		b="LibSGD";
	}else if(ver>>16==2){
		b="SoLoud";
	}else if(ver>>16==1){
		b="FMOD";
	}
	//return itoa((ver & 0xffff) / 100)+"."+itoa((ver & 0xffff) % 100)+":"+b;
	const int major = (ver & 0xffff) / 100, minor = (ver & 0xffff) % 100;
	return to_string(major)+"."+to_string(minor)+":"+b;
}

static void versInfo(){
	cout<<"Compiler version:"<<verstr(bcc_ver)<<endl;
	cout<<"Runtime version:"<<verstr(run_ver)<<endl;
	cout<<"Debugger version:"<<verstr(dbg_ver)<<endl;
	cout<<"Linker version:"<<verstr(lnk_ver)<<endl;
	exit(0);
}

static void syntaxError(const char* err="")
{
	cout << "Invalid Syntax"; 
	if (!!err) cout << ": " << err;
	cout << endl << endl;
	showHelp();
}

int _cdecl main( int argc,char *argv[] ){

	string in_file,out_file,args,forcedBlitzpath;

	bool debug=false,quiet=false,veryquiet=false,compileonly=false;
	bool dumpkeys=false,dumphelp=false,showhelp=false,dumpasm=false;
	bool versinfo=false,hook=false,ovrr=false,ide=false;

	for( int k=1;k<argc;++k ){

		string t=argv[k];
		//string n=argv[k+1];

		t=tolower(t);

		if( t=="-h" ){
			showhelp=true;
		}else if( t=="-a" ){
			dumpasm=true;
		}else if( t=="-q" ){
			quiet=true;
		}else if( t=="+q" ){
			quiet=veryquiet=true;
		}else if( t=="-c" ){
			compileonly=true;
		}else if( t=="-d" ){
			debug=true;
		}else if( t=="-k" ){
			dumpkeys=true;
		}else if( t=="+k" ){
			dumpkeys=dumphelp=true;
		}else if( t=="-v" ){
			versinfo=true;
		}else if (t=="-hook") {
			hook = true;
		}else if (t=="-blitzpath") {
			forcedBlitzpath = argv[++k];
		}else if (t=="-noinputerr") {
			ovrr = true;
		}else if (t == "-ide") {
			ide=ovrr=true;
		}else if( t=="-o" ){
			if( out_file.size() ) syntaxError("More than one output file is currently not supported.");
			if ( k==argc-1 ) syntaxError("You must specify an output file to use the \"-o\" switch.");
			//if ( n[0]=='-' ) syntaxError("You must specify an output file to use the \"-o\" switch.");
			out_file=argv[++k];
		}else{
			if ( t[0]=='-' || t[0]=='+' ) syntaxError(("Undefined Blitzcc command line switch: \""+t+"\"!").c_str());
			if ( in_file.size() ) syntaxError("More than one input file is currently not supported.");
			in_file=argv[k];
			for( ++k;k<argc;++k ){
				string t=argv[k];
				if( t.find(' ')!=string::npos ) t='\"'+t+'\"';
				if( args.size() ) args+=' ';
				args+=t;
			}
		}
	}

	if (showhelp) showHelp();

	if (const char* er = openLibs(ovrr, forcedBlitzpath)) err(er);

	if (versinfo) versInfo();

	if (in_file == "" && !ovrr)  syntaxError("An input file must be specified!");

	if (!in_file.size() && !ovrr) syntaxError("Input file does not exist or contains no information!");

	if( out_file.size() && !in_file.size() ) syntaxError();

	if( const char *er=linkLibs() ) err( er );

	if( dumpkeys ) dumpKeys( true,true,dumphelp );

	if( in_file[0]=='\"' ){
		if( in_file.size()<3 || in_file[in_file.size()-1]!='\"' ) syntaxError();
		in_file=in_file.substr( 1,in_file.size()-2 );
	}

	ifstream in( in_file.c_str() );
	if( !in && !ovrr ) err( "Unable to open input file" );

	if (!hook && debug)
	{
		cout << "WARNING: Debug compile with hook disabled is UNFINISHED FUNCTIONALITY! Continue anyways (y/n)?"<<endl;
		if (!(getchar() == 'y')) {
			cout << "Aborted!"<<endl;
			exit(0);
		}
	}

	if( !quiet ){
		showInfo();
		cout<<"Compiling \""<<in_file<<"\""<<endl;
	}

	int n=in_file.rfind( '/' );
	if( n==string::npos ) n=in_file.rfind( '\\' );
	if( n!=string::npos ){
		if( !n || in_file[n-1]==':' ) ++n;
		SetCurrentDirectory( in_file.substr(0,n).c_str() );
	}

	if (ide)
	{
		closeLibs();
		return 0;
	}

	ProgNode *prog=0;
	Environ *environ=0;
	Module *module=0;

	try{
		//parse
		if( !veryquiet ) cout<<"Parsing..."<<endl;
		Toker toker( in );
		Parser parser( toker );
		prog=parser.parse( in_file );

		//semant
		if( !veryquiet ) cout<<"Generating..."<<endl;
		environ=prog->semant( runtimeEnviron );

		//translate
		if( !veryquiet ) cout<<"Translating..."<<endl;
		qstreambuf qbuf;
		iostream asmcode( &qbuf );
		Codegen_x86 codegen( asmcode,debug );

		prog->translate( &codegen,userFuncs );

		if( dumpasm ){
			cout<<endl<<string( qbuf.data(),qbuf.size() )<<endl;
		}

		//assemble
		if( !veryquiet ) cout<<"Assembling..."<<endl;
		module=linkerLib->createModule();
		Assem_x86 assem( asmcode,module );
		assem.assemble();

	}catch( Ex &x ){

		string file='\"'+x.file+'\"';
		int row=((x.pos>>16)&65535)+1,col=(x.pos&65535)+1;
		cout<<file<<":"<<row<<":"<<col<<":"<<row<<":"<<col<<":"<<x.ex<<endl;
		exit(-1);
	}

	delete prog;

	if( out_file.size() ){
		if( !veryquiet ) cout<<"Creating executable \""<<out_file<<"\"..."<<endl;
		if( !module->createExe( out_file.c_str(),(home+"/bin/runtime.dll").c_str(),"")) {
			err( "Error creating executable: " + out_file);
		}
	}else if( !compileonly ){
		if ( hook )
		{
			//old system
			void *entry=module->link( runtimeModule );
			if( !entry ) return 0;

			HMODULE dbgHandle=0;
			Debugger *debugger=0;

			if ( debug ) {
				dbgHandle=LoadLibrary( (home+"/bin/debugger.dll").c_str() );
				if ( dbgHandle ) {
					typedef Debugger *(_cdecl*GetDebugger)( Module*,Environ* );
					GetDebugger gd=(GetDebugger)GetProcAddress( dbgHandle,"debuggerGetDebugger" );
					if( gd ) debugger=gd( module,environ );
				}
				if( !debugger ) err( "Error launching debugger" );
			}

			if (!veryquiet) cout << "Executing..." << endl;
			runtimeLib->execute((void(*)())entry, args.c_str(), debugger);

			if (dbgHandle) FreeLibrary(dbgHandle);
		}
		else {
			// trying something that hopefully makes this faster
			module->lnkModule = module;
			module->lnkEnviron = environ;
			CreateDirectoryA((home+"\\bin\\tmp\\").c_str(), NULL);
			if (debug)
			{
				if (!module->createExe((home + "\\bin\\tmp\\tmpapplication.exe").c_str(), (home + "/bin/runtime.dll").c_str(), (home + "/bin/debugger.dll").c_str())) {
					err("Error creating temporary debugged executable: " + home + "\\bin\\tmp\\tmpapplication.exe");
				}
			}
			else {
				if (!module->createExe((home + "\\bin\\tmp\\tmpapplication.exe").c_str(), (home + "/bin/runtime.dll").c_str(), "")) {
					err("Error creating temporary executable: " + home + "\\bin\\tmp\\tmpapplication.exe");
				}
			}

			ofstream environSave;
			environSave.open((home + "\\bin\\tmp\\tmpapplication.environ").c_str(), ios::out | ios::app | ios::binary);
			environSave << environ;
			environSave.close();

			if (!veryquiet) cout << "Executing..." << endl;

			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));
			if (debug)
			{
				if (!CreateProcessA((char*)(home + "\\bin\\tmp\\tmpapplication.exe").c_str(), "/DEBUG", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
					::MessageBox(0, "Unable to execute compiled temporary application!", "Blitz Basic Error", MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_TOPMOST | MB_ICONERROR);
					ExitProcess(-1);
				}
			}
			else {
				if (!CreateProcessA((char*)(home + "\\bin\\tmp\\tmpapplication.exe").c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
					::MessageBox(0, "Unable to execute compiled temporary application!", "Blitz Basic Error", MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_TOPMOST | MB_ICONERROR);
					ExitProcess(-1);
				}
			}

			//wait for tmpapplication to start
			//WaitForInputIdle(pi.hProcess, INFINITE);

			//wait for tmpapplication to end
			if (!veryquiet) cout << "Waiting for temporary application to end..." << endl;
			WaitForSingleObject(pi.hProcess, INFINITE);
			if (!veryquiet) cout << "Cleaning up..." << endl;
			remove((home + "\\bin\\tmp\\tmpapplication.exe").c_str());
			remove((home + "\\bin\\tmp\\tmpapplication.environ").c_str());
			RemoveDirectoryA((home + "\\bin\\tmp\\").c_str());

			// Close process and thread handles. 
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

		}
	}

	delete module;
	delete environ;

	closeLibs();

	if (!veryquiet) cout << "Compilation complete!" << endl;

	return 0;
}
