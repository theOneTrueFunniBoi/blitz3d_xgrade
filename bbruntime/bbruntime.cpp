#include "std.h"
#include "bbsys.h"
#include "bbruntime.h"

//const BBStr* overrideLineTrace = d_new BBStr("");
//const BBStr* overrideAddressTrace = d_new BBStr("");
//const BBStr* overrideLineTrace = (BBStr*)"";
//const BBStr* overrideAddressTrace = (BBStr*)"";
const BBStr* overrideLineTrace;
const BBStr* overrideAddressTrace;
bool overrideTrace = false;
bool overrideDef = false;

void bbEnd()
{
    RTEX(0);
}

void bbStop()
{
    gx_runtime->debugStop();
    if (!gx_runtime->idle())
        RTEX(0);
}

void bbAppTitle(BBStr* ti, BBStr* cp)
{
    gx_runtime->setTitle(*ti, *cp);
    delete ti;
    delete cp;
}

int bbExecFile(BBStr* f)
{
    string t = *f;
    delete f;
    int n = gx_runtime->execute(t);
    if (!gx_runtime->idle())
        RTEX(0);
    return n;
}

void bbDelay(int ms)
{
    if (!gx_runtime->delay(ms))
        RTEX(0);
}

int bbMilliSecs()
{
    return gx_runtime->getMilliSecs();
}

BBStr* bbCommandLine()
{
    return d_new BBStr(gx_runtime->commandLine());
}

BBStr* bbSystemProperty(BBStr* p)
{
    string t = gx_runtime->systemProperty(*p);
    delete p;
    return d_new BBStr(t);
}

BBStr* bbGetEnv(BBStr* env_var)
{
    char* p = getenv(env_var->c_str());
    BBStr* val = d_new BBStr(p ? p : "");
    delete env_var;
    return val;
}

void bbSetEnv(BBStr* env_var, BBStr* val)
{
    string t = *env_var + "=" + *val;
    putenv(t.c_str());
    delete env_var;
    delete val;
}

gxTimer* bbCreateTimer(int hertz)
{
    gxTimer* t = gx_runtime->createTimer(hertz);
    return t;
}

int bbWaitTimer(gxTimer* t)
{
    int n = t->wait();
    if (!gx_runtime->idle())
        RTEX(0);
    return n;
}

void bbFreeTimer(gxTimer* t)
{
    gx_runtime->freeTimer(t);
}

void bbDebugLog(BBStr* t)
{
    gx_runtime->debugLog(t->c_str());
    delete t;
}

std::vector<BlockTrace> blockTraces;

void _bbPushLineTrace(int line) {
    if (blockTraces.size() == 0) RTEX("FATAL: Unable to retrieve block trace! Block Trace size is Null!");
    blockTraces[blockTraces.size() - 1].lineTrace = line;
}

void _bbPushBlockTrace(const char* s) {
    try {
        printf("BLOCKTRACE OPEN ");
        printf(s);
        printf("\n");
        blockTraces.push_back(BlockTrace(string(s)));
    }
    catch (std::exception e) {
        printf(e.what()); throw e;
    }
    catch (...) {
        printf("what the fuck did you fucking do, YOU BROKE THE FUCKING BLOCK TRACE\n");
    }
}

void _bbPopBlockTrace() {
    if (blockTraces.size() == 0) RTEX("FATAL: Unable to retrieve block trace! Block Trace size is Null!");
    printf("BLOCKTRACE CLOSE ");
    blockTraces[blockTraces.size() - 1].file += ", line " + std::to_string(blockTraces[blockTraces.size() - 1].lineTrace);
    printf(blockTraces[blockTraces.size() - 1].file.c_str());
    printf("\n");
    blockTraces.pop_back();
}

BBStr* bbGetLineTrace()
{
    string retVal = "";
    for (int i = 0; i < blockTraces.size(); i++) {
        retVal = blockTraces[i].file + ", line " + to_string(blockTraces[i].lineTrace) + "\n" + retVal;
    }
    return d_new BBStr(retVal);
}

BBStr* bbGetAddressTrace()
{
    string retVal = "";
    string tmp = "";

    void* array[10];
    unsigned short frames;
    SYMBOL_INFO* symbol;
    HANDLE process = GetCurrentProcess();

    SymInitialize(process, NULL, TRUE);
    frames = CaptureStackBackTrace(0, 10, array, NULL);
    symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (unsigned short i = 0; i < frames; i++)
    {
        SymFromAddr(process, (DWORD64)(array[i]), 0, symbol);
        tmp += "  " + (string)symbol->Name + " - 0x" + to_string(symbol->Address);
    }
    retVal += tmp;
    free(symbol);

    /*unsigned char c;
    __asm movb c, eax
    retVal = "eax=" + c;
    __asm movb c, ebx
    retVal += " ebx=" + c;
    __asm movb c, ecx
    retVal += " ecx=" + c;
    __asm movb c, edx
    retVal += " edx=" + c;
    __asm movb c, esi
    retVal += " esi=" + c;
    __asm movb c, edi
    retVal += " edi=" + c;
    __asm movb c, esp
    retVal += " esp=" + c;
    __asm movb c, ebp
    retVal += " ebp=" + c;
    __asm movb c, eip
    retVal += " eip=" + c;
    __asm movb c, al
    retVal += " al=" + c;
    __asm movb c, cs
    retVal += " cs=" + c;
    __asm movb c, ds
    retVal += " ds=" + c;
    __asm movb c, ss
    retVal += " ss=" + c;*/
    return d_new BBStr(retVal);
}

void bbRuntimeError(BBStr* str, int type)
{
    string t = *str;
    delete str;

    string tmp = "";
    BBStr tmp2 = "";

    static char err[512];

    HANDLE process = GetCurrentProcess();

    switch (type)
    {

        // prev-stacktrace (after furthur inspection, this really has no difference, gonna replace with default)
    //case 1:
    //    if (t.size() > 512) t[512] = 0;
    //    strcpy(err, t.c_str());
    //    overrideLineTrace = bbGetLineTrace();
    //    overrideAddressTrace = bbGetAddressTrace();
    //    overrideTrace = true;
    //    RTEX(err);
    //    break;

        // no stacktrace
    case 2:
        if (t.size() > 512) t[512] = 0;
        strcpy(err, t.c_str());
        //bbEx::overrideTrace = true;
        overrideTrace = true;
        RTEX(err);
        break;

        // pre-stacktrace
    case 3:
        if (t.size() > 512) t[512] = 0;
        strcpy(err, t.c_str());
        overrideDef = true;
        RTEX(err);
        break;

        // default, with stacktrace
    default:
        if (t.size() > 512) t[512] = 0;
        strcpy(err, t.c_str());
        RTEX(err);
        break;
    }
}

void bbKaboom()
{
    *(int*)0 = 0; //nuke the fuck out of b3d, to test exception handling
}

void _bbDebugStmt(int pos, const char* file)
{
    gx_runtime->debugStmt(pos, file);
    if (!gx_runtime->idle())
        RTEX(0);
}

void _bbDebugEnter(void* frame, void* env, const char* func)
{
    gx_runtime->debugEnter(frame, env, func);
}

void _bbDebugLeave()
{
    gx_runtime->debugLeave();
}

bool basic_create();
bool basic_destroy();
void basic_link(void (*rtSym)(const char* sym, void* pc));

bool math_create();
bool math_destroy();
void math_link(void (*rtSym)(const char* sym, void* pc));

bool string_create();
bool string_destroy();
void string_link(void (*rtSym)(const char* sym, void* pc));

bool stream_create();
bool stream_destroy();
void stream_link(void (*rtSym)(const char* sym, void* pc));

bool sockets_create();
bool sockets_destroy();
void sockets_link(void (*rtSym)(const char* sym, void* pc));

bool filesystem_create();
bool filesystem_destroy();
void filesystem_link(void (*rtSym)(const char* sym, void* pc));

bool bank_create();
bool bank_destroy();
void bank_link(void (*rtSym)(const char* sym, void* pc));

bool userlibs_create();
void userlibs_destroy();
void userlibs_link(void (*rtSym)(const char* sym, void* pc));

#if BB_BLITZ3D_ENABLED

bool input_create();
bool input_destroy();
void input_link(void (*rtSym)(const char* sym, void* pc));

bool graphics_create();
bool graphics_destroy();
void graphics_link( void (*rtSym)( const char *sym,void *pc ) );

bool audio_create();
bool audio_destroy();
void audio_link(void (*rtSym)(const char* sym, void* pc));

bool blitz3d_create();
bool blitz3d_destroy();
void blitz3d_link( void (*rtSym)( const char *sym,void *pc ) );

#elif BB_LIBSGD_ENABLED

bool sgd_create();
bool sgd_destroy();
bool sgd_link(void (*rtSym)(const char* sym, void* pc));

#endif

void bbruntime_link(void (*rtSym)(const char* sym, void* pc))
{
    rtSym("End", bbEnd);
    rtSym("Stop", bbStop);
    rtSym("AppTitle$title$close_prompt=\"\"", bbAppTitle);
    rtSym("RuntimeError$message%type=0", bbRuntimeError);
    rtSym("ExecFile$command", bbExecFile);
    rtSym("Delay%millisecs", bbDelay);
    rtSym("%MilliSecs", bbMilliSecs);
    rtSym("$CommandLine", bbCommandLine);
    rtSym("$SystemProperty$property", bbSystemProperty);
    rtSym("$GetEnv$env_var", bbGetEnv);
    rtSym("SetEnv$env_var$value", bbSetEnv);

    rtSym("%CreateTimer%hertz", bbCreateTimer);
    rtSym("%WaitTimer%timer", bbWaitTimer);
    rtSym("FreeTimer%timer", bbFreeTimer);
    rtSym("DebugLog$text", bbDebugLog);
    rtSym("$ErrorLog",bbErrorLog);

    rtSym("$GetFullLineTrace", bbGetLineTrace);
    rtSym("$GetFullAddressTrace", bbGetAddressTrace);
    rtSym("_bbPushLineTrace", _bbPushLineTrace);
    rtSym("_bbPushBlockTrace", _bbPushBlockTrace);
    rtSym("_bbPopBlockTrace", _bbPopBlockTrace);

    rtSym("Kaboom", bbKaboom);

    rtSym("_bbDebugStmt", _bbDebugStmt);
    rtSym("_bbDebugEnter", _bbDebugEnter);
    rtSym("_bbDebugLeave", _bbDebugLeave);

    basic_link(rtSym);
    math_link(rtSym);
    string_link(rtSym);
    stream_link(rtSym);
    sockets_link(rtSym);
    filesystem_link(rtSym);
    bank_link(rtSym);
	userlibs_link(rtSym);
#if BB_BLITZ3D_ENABLED
	input_link(rtSym);
	graphics_link( rtSym );
    audio_link(rtSym);
	blitz3d_link( rtSym );
#elif BB_LIBSGD_ENABLED
    sgd_link(rtSym);
#endif
}

//start up error
static void sue(const char* t)
{
    string p = string("Startup Error: ") + t;
    gx_runtime->debugInfo(p.c_str());
}

bool bbruntime_create()
{
    if (basic_create())
    {
        if (math_create())
        {
            if (string_create())
            {
                if (stream_create())
                {
                    if (sockets_create())
                    {
                        if (filesystem_create())
                        {
                            if (bank_create())
                            {
								if (userlibs_create())
								{
#if BB_BLITZ3D_ENABLED
									if( graphics_create() )
									{
										if (input_create())
										{
											if (audio_create())
											{
												if( blitz3d_create() )
												{
													return true;

												}else sue("blitz3d_create failed");
												audio_destroy();
											}
											else sue("audio_create failed");
											input_destroy();
										}
										else sue("input_create failed");
										graphics_destroy();
									}else sue( "graphics_create failed" );
#elif BB_LIBSGD_ENABLED
									if (sgd_create()) {
										return true;
									}else sue("sgd_create failed");
#endif
									userlibs_destroy();
								}else sue("userlibs_create failed");
                                bank_destroy();
                            }
                            else sue("bank_create failed");
                            filesystem_destroy();
                        }
                        else sue("filesystem_create failed");
                        sockets_destroy();
                    }
                    else sue("sockets_create failed");
                    stream_destroy();
                }
                else sue("stream_create failed");
                string_destroy();
            }
            else sue("string_create failed");
            math_destroy();
        }
        else sue("math_create failed");
        basic_destroy();
    }
    else sue("basic_create failed");
    return false;
}

bool bbruntime_destroy()
{
    printf("userlibs\n");
    userlibs_destroy();
#if BB_BLITZ3D_ENABLED
    printf("blitz3d\n");
    blitz3d_destroy();
    printf("audio\n");
    audio_destroy();
    printf("input\n");
    input_destroy();
    printf("graphics\n");
	graphics_destroy();
#elif BB_LIBSGD_ENABLED
    printf("sgd\n");
	sgd_destroy();
#endif
    printf("bank\n");
    bank_destroy();
    printf("filesystem\n");
    filesystem_destroy();
    printf("sockets\n");
    sockets_destroy();
    printf("stream\n");
    stream_destroy();
    printf("string\n");
    string_destroy();
    printf("math\n");
    math_destroy();
    printf("basic\n");
    basic_destroy();
    printf("everything don gon be destroyed\n");
    return true;
}

const char* bbruntime_run(gxRuntime* rt, void (*pc)(), bool dbg)
{
    debug = dbg;
    gx_runtime = rt;

    const char* t = 0;
    try
    {
        if (!bbruntime_create()) return "FATAL: Failed to initialize BlitzApplication Runtime!";
        if (!gx_runtime->idle())
            RTEX(0);
        pc();
        gx_runtime->debugInfo("INFO: BlitzApplication has exited.");
    }
    catch (bbEx x)
    {
        t = x.err;
    }
    catch (exception e) {
        t = e.what();
    }
    catch (...) {
        t = "FATAL: Unknown, or non-standard exception thrown!";
    }
    bbruntime_destroy();
    return t;
}

void bbruntime_panic(const char* err)
{
    RTEX(err);
}