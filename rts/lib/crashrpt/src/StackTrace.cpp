/*----------------------------------------------------------------------
   John Robbins - Microsoft Systems Journal Bugslayer Column - Feb 99
----------------------------------------------------------------------*/
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>

// Force imagehlp in.
#include <imagehlp.h>


#include "StackTrace.h"
#include "SymbolEngine.h"

// 4710: inline function not inlined
#pragma warning(disable: 4710)
#pragma warning(disable: 4786)
#pragma warning(push, 3)
#include <map>
#pragma warning(pop)

/*//////////////////////////////////////////////////////////////////////
                           File Scope Globals
//////////////////////////////////////////////////////////////////////*/

// The symbol engine. Indexed by process-id so there is no collision between processes.
#pragma warning(push, 3)
typedef std::map<DWORD, CSymbolEngine> TSymbolEngineMap;
static TSymbolEngineMap g_cSymMap;
#pragma warning(pop)

static CSymbolEngine & GetSymbolEngine()
{
	TSymbolEngineMap::iterator	iter;
	iter = g_cSymMap.find(GetCurrentProcessId());
	if (iter == g_cSymMap.end()) {
		CSymbolEngine	cSym;
	    HANDLE hProcess = GetCurrentProcess ( ) ;
        DWORD dwOpts = SymGetOptions ( ) ;

        // Turn on load lines.
        SymSetOptions ( dwOpts                |
                        SYMOPT_LOAD_LINES      ) ;

        if ( FALSE == g_cSymMap[GetCurrentProcessId()].SymInitialize ( hProcess ,
                                             NULL     ,
                                             FALSE     ) )
            {
            OutputDebugString ( "DiagAssert : Unable to initialize the "
                    "symbol engine!!!\n" ) ;
#ifdef _DEBUG
            DebugBreak ( ) ;
#endif
            }
      }
	return g_cSymMap[GetCurrentProcessId()];
}

static DWORD __stdcall GetModBase ( HANDLE hProcess , DWORD dwAddr )
{
    // Check in the symbol engine first.
    IMAGEHLP_MODULE stIHM ;
	CSymbolEngine	& cSym = GetSymbolEngine();

    // This is what the MFC stack trace routines forgot to do so their
    //  code will not get the info out of the symbol engine.
    stIHM.SizeOfStruct = sizeof ( IMAGEHLP_MODULE ) ;

    if ( cSym.SymGetModuleInfo ( dwAddr , &stIHM ) )
    {
        return ( stIHM.BaseOfImage ) ;
    }
    else
    {
        // Let's go fishing.
        MEMORY_BASIC_INFORMATION stMBI ;

        if ( 0 != VirtualQueryEx ( hProcess         ,
                                   (LPCVOID)dwAddr  ,
                                   &stMBI           ,
                                   sizeof ( stMBI )  ) )
        {
            // Try and load it.
            DWORD dwNameLen = 0 ;
            TCHAR szFile[ MAX_PATH ] ;
            szFile[0] = '\0';
            dwNameLen = GetModuleFileName ( (HINSTANCE)
                                                stMBI.AllocationBase ,
                                            szFile                   ,
                                            MAX_PATH                  );
            HANDLE hFile = NULL ;

            if ( 0 != dwNameLen )
            {
                hFile = CreateFile ( szFile       ,
                                     GENERIC_READ    ,
                                     FILE_SHARE_READ ,
                                     NULL            ,
                                     OPEN_EXISTING   ,
                                     0               ,
                                     0                ) ;
            }
#ifdef NOTDEF_DEBUG
            DWORD dwRet =
#endif
            cSym.SymLoadModule ( hFile                            ,
                                  ( dwNameLen ? szFile : NULL )    ,
                                   NULL                             ,
                                   (DWORD)stMBI.AllocationBase      ,
                                   0                                 ) ;
			::CloseHandle(hFile);
#ifdef NOTDEF_DEBUG
            if ( 0 == dwRet )
            {
                ATLTRACE ( "SymLoadModule failed : 0x%08X\n" ,
                        GetLastError ( )                   ) ;
            }
#endif  // _DEBUG
            return ( (DWORD)stMBI.AllocationBase ) ;
        }
    }
    return ( 0 ) ;
}

static void PrintAddress (DWORD address, const char *ImageName,
									  const char *FunctionName, DWORD functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp,
									  void * /* data, unused */ )
{
    static char buffer [ MAX_PATH*2 + 512 ];
   LPTSTR pCurrPos = buffer ;
    // Always stick the address in first.
    pCurrPos += _snprintf ( pCurrPos ,  sizeof buffer - (pCurrPos - buffer), ( "0x%08X " ) , address ) ;

	if (ImageName != NULL) {
		LPTSTR szName = strchr ( ImageName ,  ( '\\' ) ) ;
		if ( NULL != szName ) {
			szName++ ;
		} else {
			szName = const_cast<char *>(ImageName) ;
		}
		pCurrPos += _snprintf ( pCurrPos ,  sizeof buffer - (pCurrPos - buffer), ( "%s: " ) , szName ) ;
	} else {
        pCurrPos += _snprintf ( pCurrPos , sizeof buffer - (pCurrPos - buffer),  ( "<unknown module>: " ) );
	}
	if (FunctionName != NULL) {
        if ( 0 == functionDisp ) {
            pCurrPos += _snprintf ( pCurrPos , sizeof buffer - (pCurrPos - buffer),  ( "%s" ) , FunctionName);
		} else {
            pCurrPos += _snprintf ( pCurrPos               , sizeof buffer - (pCurrPos - buffer), 
                                    ( "%s + %d bytes" ) ,
                                   FunctionName             ,
                                   functionDisp                  ) ;
		}
		if (Filename != NULL) {
            // Put this on the next line and indented a bit.
			pCurrPos += _snprintf( pCurrPos, sizeof buffer - (pCurrPos - buffer), "-\n");
            OutputDebugString(buffer);
            pCurrPos = buffer;
            pCurrPos += _snprintf ( pCurrPos                  , sizeof buffer - (pCurrPos - buffer), 
                                   ( "\t\t%s, Line %d" ) ,
                                  Filename             ,
                                  LineNumber            ) ;
            if ( 0 != lineDisp )
                  {
                pCurrPos += _snprintf ( pCurrPos             , sizeof buffer - (pCurrPos - buffer), 
                                        ( " + %d bytes" ) ,
                                       lineDisp                ) ;
                  }
		}
	} else {
        pCurrPos += _snprintf ( pCurrPos , sizeof buffer - (pCurrPos - buffer),  ( "<unknown symbol>" ) ) ;
	}
    // Tack on a CRLF.
    pCurrPos += _snprintf ( pCurrPos , sizeof buffer - (pCurrPos - buffer),  ( "\n" ) ) ;
    OutputDebugString ( buffer );
}


void AddressToSymbol(DWORD dwAddr, TraceCallbackFunction pFunction, LPVOID data)
{
    char szTemp [ MAX_PATH + sizeof ( IMAGEHLP_SYMBOL ) ] ;

    PIMAGEHLP_SYMBOL pIHS = (PIMAGEHLP_SYMBOL)&szTemp ;

    IMAGEHLP_MODULE stIHM ;
    IMAGEHLP_LINE stIHL ;

 	bool haveModule = false;
	bool haveFunction = false;
	bool haveLine = false;

	CSymbolEngine & cSym = GetSymbolEngine();


    ZeroMemory ( pIHS , MAX_PATH + sizeof ( IMAGEHLP_SYMBOL ) ) ;
    ZeroMemory ( &stIHM , sizeof ( IMAGEHLP_MODULE ) ) ;
    ZeroMemory ( &stIHL , sizeof ( IMAGEHLP_LINE ) ) ;

    pIHS->SizeOfStruct = sizeof ( IMAGEHLP_SYMBOL ) ;
    pIHS->Address = dwAddr ;
    pIHS->MaxNameLength = MAX_PATH ;

    stIHM.SizeOfStruct = sizeof ( IMAGEHLP_MODULE ) ;


    // Get the module name.
	haveModule = 0 != cSym.SymGetModuleInfo ( dwAddr , &stIHM );

    // Get the function.
    DWORD dwFuncDisp=0 ;
	DWORD dwLineDisp=0;
    if ( 0 != cSym.SymGetSymFromAddr ( dwAddr , &dwFuncDisp , pIHS ) )
      {
		haveFunction = true;


        // If I got a symbol, give the source and line a whirl.


        stIHL.SizeOfStruct = sizeof ( IMAGEHLP_LINE ) ;

        haveLine = 0 != cSym.SymGetLineFromAddr ( dwAddr  ,
                                              &dwLineDisp ,
                                              &stIHL   );
      }
	if (pFunction != NULL) {
		pFunction(dwAddr, haveModule ? stIHM.ImageName : NULL,
			haveFunction ? pIHS->Name : NULL, dwFuncDisp,
			haveLine ? stIHL.FileName : NULL, haveLine ? stIHL.LineNumber : 0, dwLineDisp,
			data);
	}
}

void DoStackTrace ( int numSkip, int depth, TraceCallbackFunction pFunction, CONTEXT *pContext, LPVOID data )
{
    HANDLE hProcess = GetCurrentProcess ( ) ;

	if (pFunction == NULL) {
		pFunction = PrintAddress;
	}

    // The symbol engine is initialized so do the stack walk.

    // The thread information - if not supplied.
    CONTEXT    stCtx  ;
	if (pContext == NULL) {

		stCtx.ContextFlags = CONTEXT_FULL ;

		if ( GetThreadContext ( GetCurrentThread ( ) , &stCtx ) )
		  {
			pContext = &stCtx;
		}
	}
	if (pContext != NULL) {
        STACKFRAME stFrame ;
        DWORD      dwMachine ;

        ZeroMemory ( &stFrame , sizeof ( STACKFRAME ) ) ;

        stFrame.AddrPC.Mode = AddrModeFlat ;

#if defined (_M_IX86)
        dwMachine                = IMAGE_FILE_MACHINE_I386 ;
        stFrame.AddrPC.Offset    = pContext->Eip    ;
        stFrame.AddrStack.Offset = pContext->Esp    ;
        stFrame.AddrStack.Mode   = AddrModeFlat ;
        stFrame.AddrFrame.Offset = pContext->Ebp    ;
        stFrame.AddrFrame.Mode   = AddrModeFlat ;

#elif defined (_M_ALPHA)
        dwMachine                = IMAGE_FILE_MACHINE_ALPHA ;
        stFrame.AddrPC.Offset    = (unsigned long)pContext->Fir ;
#else
#error ( "Unknown machine!" )
#endif

        // Loop for the first <depth> stack elements.
        for ( int i = 0 ; i < depth ; i++ )
            {
            if ( FALSE == StackWalk ( dwMachine              ,
                                      hProcess               ,
                                      hProcess               ,
                                      &stFrame               ,
                                      pContext               ,
                                      NULL                   ,
                                      SymFunctionTableAccess ,
                                      GetModBase             ,
                                      NULL                    ) )
                  {
                break ;
                  }
            if ( i > numSkip )
                  {
                // Also check that the address is not zero.  Sometimes
                //  StackWalk returns TRUE with a frame of zero.
                if ( 0 != stFrame.AddrPC.Offset )
                        {
			            AddressToSymbol ( stFrame.AddrPC.Offset-1, pFunction, data ) ;
                        }
                  }
            }

      }
}
