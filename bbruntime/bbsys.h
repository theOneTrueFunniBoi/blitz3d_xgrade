
#ifndef BBSYS_H
#define BBSYS_H

#include "basic.h"
#include "../gxruntime/gxruntime.h"
#include <DbgHelp.h>

extern bool debug;
extern gxRuntime *gx_runtime;

extern std::vector<std::string> errorLog;
BBStr * bbErrorLog( );

struct bbEx{
	const char *err;
	bbEx( const char *e ):err(e){
        if (e) {
            string panicStr = e;
            panicStr += "\n\nStack line trace:\n";
            try {
                string tmp = "";
                for (int i = 0; i < blockTraces.size(); i++) {
                    tmp = " -   " + blockTraces[i].file + ", line " + to_string(blockTraces[i].lineTrace) + "\n" + tmp;
                }
                panicStr += tmp;

                void* array[10];
                unsigned short frames;
                SYMBOL_INFO* symbol;
                HANDLE process = GetCurrentProcess();

                SymInitialize(process, NULL, TRUE);
                frames = CaptureStackBackTrace(0, 10, array, NULL);
                symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
                symbol->MaxNameLen = 255;
                symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

                panicStr += "\n\nStack address backtrace:\n";
                tmp = "";

                for (unsigned short i = 0; i < frames; i++)
                {
                    SymFromAddr(process, (DWORD64)(array[i]), 0, symbol);
                    tmp+=" -   "+(string)symbol->Name+" - 0x"+to_string(symbol->Address)+"\n";
                }
                panicStr += tmp;
                free(symbol);

                /*unsigned char c;
                __asm movb c, eax
                panicStr += " - eax=" + c;
                __asm movb c, ebx
                panicStr += " ebx=" + c;
                __asm movb c, ecx
                panicStr += " ecx=" + c;
                __asm movb c, edx
                panicStr += " edx=" + c;
                __asm movb c, esi
                panicStr += " esi=" + c;
                __asm movb c, edi
                panicStr += " edi=" + c;
                __asm movb c, esp
                panicStr += "\n - esp=" + c;
                __asm movb c, ebp
                panicStr += " ebp=" + c;
                __asm movb c, eip
                panicStr += " eip=" + c;
                __asm movb c, al
                panicStr += " al=" + c;
                __asm movb c, cs
                panicStr += " cs=" + c;
                __asm movb c, ds
                panicStr += " ds=" + c;
                __asm movb c, ss
                panicStr += "\n - ss=" + c;*/
            }
            catch (exception e) {
                panicStr += "ERROR RETRIEVING FULL STACKTRACE: ";
                panicStr += e.what();
                panicStr += "\n";
            }
            catch (...) {
                panicStr += "ERROR RETRIEVING FULL STACKTRACE\n";
            }
            gx_runtime->debugError(panicStr.c_str());
        }
	}
};

#define RTEX( _X_ ) throw bbEx( _X_ );

#endif