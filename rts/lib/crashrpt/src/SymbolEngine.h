/*----------------------------------------------------------------------
"Debugging Applications" (Microsoft Press)
Copyright (c) 1997-2000 John Robbins -- All rights reserved.
------------------------------------------------------------------------
This class is a paper-thin layer around the DBGHELP.DLL symbol engine.

This class wraps only those functions that take the unique
HANDLE value. Other DBGHELP.DLL symbol engine functions are global in
scope, so I didn’t wrap them with this class.

------------------------------------------------------------------------
Compilation Defines:

DO_NOT_WORK_AROUND_SRCLINE_BUG - If defined, the class will NOT work
                                 around the SymGetLineFromAddr bug where
                                 PDB file lookups fail after the first
                                 lookup.
USE_BUGSLAYERUTIL - If defined, the class will have another
                    initialization method, BSUSymInitialize, which will
                    use BSUSymInitialize from BUGSLAYERUTIL.DLL to
                    initialize the symbol engine and allow the invade
                    process flag to work for all Win32 operating systems.
                    If you use this define, you must use
                    BUGSLAYERUTIL.H to include this file.
----------------------------------------------------------------------*/

#ifndef _SYMBOLENGINE_H
#define _SYMBOLENGINE_H

// You could include either IMAGEHLP.DLL or DBGHELP.DLL.
#include "imagehlp.h"
#include <tchar.h>

// Include these in case the user forgets to link against them.
#pragma comment (lib,"dbghelp.lib")
#pragma comment (lib,"version.lib")

// The great Bugslayer idea of creating wrapper classes on structures
// that have size fields came from fellow MSJ columnist, Paul DiLascia.
// Thanks, Paul!

// I didn’t wrap IMAGEHLP_SYMBOL because that is a variable-size
// structure.

// The IMAGEHLP_MODULE wrapper class
struct CImageHlp_Module : public IMAGEHLP_MODULE
{
    CImageHlp_Module ( )
    {
        memset ( this , NULL , sizeof ( IMAGEHLP_MODULE ) ) ;
        SizeOfStruct = sizeof ( IMAGEHLP_MODULE ) ;
    }
} ;

// The IMAGEHLP_LINE wrapper class
struct CImageHlp_Line : public IMAGEHLP_LINE
{
    CImageHlp_Line ( )
    {
        memset ( this , NULL , sizeof ( IMAGEHLP_LINE ) ) ;
        SizeOfStruct = sizeof ( IMAGEHLP_LINE ) ;
    }
} ;

// The symbol engine class
class CSymbolEngine
{
/*----------------------------------------------------------------------
                  Public Construction and Destruction
----------------------------------------------------------------------*/
public      :
    // To use this class, call the SymInitialize member function to
    // initialize the symbol engine and then use the other member
    // functions in place of their corresponding DBGHELP.DLL functions.
    CSymbolEngine ( void )
    {
    }

    virtual ~CSymbolEngine ( void )
    {
    }

/*----------------------------------------------------------------------
                  Public Helper Information Functions
----------------------------------------------------------------------*/
public      :

    // Returns the file version of DBGHELP.DLL being used.
    //  To convert the return values into a readable format:
    //  wsprintf ( szVer                  ,
    //              ( "%d.%02d.%d.%d" ) ,
    //             HIWORD ( dwMS )        ,
    //             LOWORD ( dwMS )        ,
    //             HIWORD ( dwLS )        ,
    //             LOWORD ( dwLS )         ) ;
    //  szVer will contain a string like: 5.00.1878.1
    BOOL GetImageHlpVersion ( DWORD & dwMS , DWORD & dwLS )
    {
        return( GetInMemoryFileVersion (  ( "DBGHELP.DLL" ) ,
                                         dwMS                 ,
                                         dwLS                  ) ) ;
    }

    BOOL GetDbgHelpVersion ( DWORD & dwMS , DWORD & dwLS )
    {
        return( GetInMemoryFileVersion (  ( "DBGHELP.DLL" ) ,
                                         dwMS                 ,
                                         dwLS                  ) ) ;
    }

    // Returns the file version of the PDB reading DLLs
    BOOL GetPDBReaderVersion ( DWORD & dwMS , DWORD & dwLS )
    {
        // First try MSDBI.DLL.
        if ( TRUE == GetInMemoryFileVersion (  ( "MSDBI.DLL" ) ,
                                              dwMS               ,
                                              dwLS                ) )
        {
            return ( TRUE ) ;
        }
        else if ( TRUE == GetInMemoryFileVersion (  ( "MSPDB60.DLL" ),
                                                   dwMS                ,
                                                   dwLS               ))
        {
            return ( TRUE ) ;
        }
        // Just fall down to MSPDB50.DLL.
        return ( GetInMemoryFileVersion (  ( "MSPDB50.DLL" ) ,
                                          dwMS                 ,
                                          dwLS                  ) ) ;
    }

    // The worker function used by the previous two functions
    BOOL GetInMemoryFileVersion ( LPCTSTR szFile ,
                                  DWORD & dwMS   ,
                                  DWORD & dwLS    )
    {
        HMODULE hInstIH = GetModuleHandle ( szFile ) ;

        // Get the full filename of the loaded version.
        TCHAR szImageHlp[ MAX_PATH ] ;
        GetModuleFileName ( hInstIH , szImageHlp , MAX_PATH ) ;

        dwMS = 0 ;
        dwLS = 0 ;

        // Get the version information size.
        DWORD dwVerInfoHandle ;
        DWORD dwVerSize       ;

        dwVerSize = GetFileVersionInfoSize ( szImageHlp       ,
                                             &dwVerInfoHandle  ) ;
        if ( 0 == dwVerSize )
        {
            return ( FALSE ) ;
        }

        // Got the version size, now get the version information.
        LPVOID lpData = (LPVOID)new TCHAR [ dwVerSize ] ;
        if ( FALSE == GetFileVersionInfo ( szImageHlp       ,
                                           dwVerInfoHandle  ,
                                           dwVerSize        ,
                                           lpData            ) )
        {
            delete [] lpData ;
            return ( FALSE ) ;
        }

        VS_FIXEDFILEINFO * lpVerInfo ;
        UINT uiLen ;
        BOOL bRet = VerQueryValue ( lpData              ,
                                     ( "\\" )         ,
                                    (LPVOID*)&lpVerInfo ,
                                    &uiLen               ) ;
        if ( TRUE == bRet )
        {
            dwMS = lpVerInfo->dwFileVersionMS ;
            dwLS = lpVerInfo->dwFileVersionLS ;
        }

        delete [] lpData ;

        return ( bRet ) ;
    }

/*----------------------------------------------------------------------
                   Public Initialization and Cleanup
----------------------------------------------------------------------*/
public      :

    BOOL SymInitialize ( IN HANDLE   hProcess       ,
                         IN LPSTR    UserSearchPath ,
                         IN BOOL     fInvadeProcess  )
    {
        m_hProcess = hProcess ;
        return ( ::SymInitialize ( hProcess       ,
                                   UserSearchPath ,
                                   fInvadeProcess  ) ) ;
    }

#ifdef USE_BUGSLAYERUTIL
    BOOL BSUSymInitialize ( DWORD  dwPID          ,
                            HANDLE hProcess       ,
                            PSTR   UserSearchPath ,
                            BOOL   fInvadeProcess  )
    {
        m_hProcess = hProcess ;
        return ( ::BSUSymInitialize ( dwPID          ,
                                      hProcess       ,
                                      UserSearchPath ,
                                      fInvadeProcess  ) ) ;
    }
#endif  // USE_BUGSLAYERUTIL
    BOOL SymCleanup ( void )
    {
        return ( ::SymCleanup ( m_hProcess ) ) ;
    }

/*----------------------------------------------------------------------
                       Public Module Manipulation
----------------------------------------------------------------------*/
public      :

    BOOL SymEnumerateModules ( IN PSYM_ENUMMODULES_CALLBACK
                                                    EnumModulesCallback,
                               IN PVOID             UserContext )
    {
        return ( ::SymEnumerateModules ( m_hProcess           ,
                                         EnumModulesCallback  ,
                                         UserContext           ) ) ;
    }

    BOOL SymLoadModule ( IN  HANDLE hFile       ,
                         IN  PSTR   ImageName   ,
                         IN  PSTR   ModuleName  ,
                         IN  DWORD  BaseOfDll   ,
                         IN  DWORD  SizeOfDll    )
    {
        return ( ::SymLoadModule ( m_hProcess   ,
                                   hFile        ,
                                   ImageName    ,
                                   ModuleName   ,
                                   BaseOfDll    ,
                                   SizeOfDll     ) ) ;
    }

    BOOL EnumerateLoadedModules ( IN PENUMLOADED_MODULES_CALLBACK
                                              EnumLoadedModulesCallback,
                                  IN PVOID         UserContext       )
    {
        return ( ::EnumerateLoadedModules ( m_hProcess                ,
                                            EnumLoadedModulesCallback ,
                                            UserContext              ));
    }

    BOOL SymUnloadModule ( IN  DWORD BaseOfDll )
    {
        return ( ::SymUnloadModule ( m_hProcess , BaseOfDll ) ) ;
    }

    BOOL SymGetModuleInfo ( IN  DWORD            dwAddr     ,
                            OUT PIMAGEHLP_MODULE ModuleInfo  )
    {
        return ( ::SymGetModuleInfo ( m_hProcess    ,
                                      dwAddr        ,
                                      ModuleInfo     ) ) ;
    }

    DWORD SymGetModuleBase ( IN DWORD dwAddr )
    {
        return ( ::SymGetModuleBase ( m_hProcess , dwAddr ) ) ;
    }

/*----------------------------------------------------------------------
                       Public Symbol Manipulation
----------------------------------------------------------------------*/
public      :

    BOOL SymEnumerateSymbols (IN DWORD                        BaseOfDll,
                              IN PSYM_ENUMSYMBOLS_CALLBACK
                                                    EnumSymbolsCallback,
                              IN PVOID                     UserContext )
    {
        return ( ::SymEnumerateSymbols ( m_hProcess          ,
                                         BaseOfDll           ,
                                         EnumSymbolsCallback ,
                                         UserContext          ) ) ;
    }

    BOOL SymGetSymFromAddr ( IN  DWORD               dwAddr          ,
                             OUT PDWORD              pdwDisplacement ,
                             OUT PIMAGEHLP_SYMBOL    Symbol           )
    {
        return ( ::SymGetSymFromAddr ( m_hProcess       ,
                                       dwAddr           ,
                                       pdwDisplacement  ,
                                       Symbol            ) ) ;
    }

    BOOL SymGetSymFromName ( IN  LPSTR            Name   ,
                             OUT PIMAGEHLP_SYMBOL Symbol  )
    {
        return ( ::SymGetSymFromName ( m_hProcess ,
                                       Name       ,
                                       Symbol      ) ) ;
    }

    BOOL SymGetSymNext ( IN OUT PIMAGEHLP_SYMBOL Symbol )
    {
        return ( ::SymGetSymNext ( m_hProcess , Symbol ) ) ;
    }

    BOOL SymGetSymPrev ( IN OUT PIMAGEHLP_SYMBOL Symbol )
    {
        return ( ::SymGetSymPrev ( m_hProcess , Symbol ) ) ;
    }

/*----------------------------------------------------------------------
                    Public Source Line Manipulation
----------------------------------------------------------------------*/
public      :

    BOOL SymGetLineFromAddr ( IN  DWORD          dwAddr          ,
                              OUT PDWORD         pdwDisplacement ,
                              OUT PIMAGEHLP_LINE Line             )
    {

#ifdef DO_NOT_WORK_AROUND_SRCLINE_BUG
        // Just pass along the values returned by the main function.
        return ( ::SymGetLineFromAddr ( m_hProcess      ,
                                        dwAddr          ,
                                        pdwDisplacement ,
                                        Line             ) ) ;

#else
        // The problem is that the symbol engine finds only those source
        // line addresses (after the first lookup) that fall exactly on
        // a zero displacement. I’ll walk backward 100 bytes to
        // find the line and return the proper displacement.
        DWORD dwTempDis = 0 ;
        while ( FALSE == ::SymGetLineFromAddr ( m_hProcess          ,
                                                dwAddr - dwTempDis  ,
                                                pdwDisplacement     ,
                                                Line                 ) )
        {
            dwTempDis += 1 ;
            if ( 100 == dwTempDis )
            {
                return ( FALSE ) ;
            }
        }
        // I found it and the source line information is correct, so
        // change the displacement if I had to search backward to find
        // the source line.
        if ( 0 != dwTempDis )
        {
            *pdwDisplacement = dwTempDis ;
        }
        return ( TRUE ) ;
#endif // DO_NOT_WORK_AROUND_SRCLINE_BUG
    }

    BOOL SymGetLineFromName ( IN     LPSTR          ModuleName      ,
                              IN     LPSTR          FileName        ,
                              IN     DWORD          dwLineNumber    ,
                              OUT    PLONG          plDisplacement  ,
                              IN OUT PIMAGEHLP_LINE Line             )
    {
        return ( ::SymGetLineFromName ( m_hProcess       ,
                                        ModuleName       ,
                                        FileName         ,
                                        dwLineNumber     ,
                                        plDisplacement   ,
                                        Line              ) ) ;
    }

    BOOL SymGetLineNext ( IN OUT PIMAGEHLP_LINE Line )
    {
        return ( ::SymGetLineNext ( m_hProcess , Line ) ) ;
    }

    BOOL SymGetLinePrev ( IN OUT PIMAGEHLP_LINE Line )
    {
        return ( ::SymGetLinePrev ( m_hProcess , Line ) ) ;
    }

    BOOL SymMatchFileName ( IN  LPSTR   FileName        ,
                            IN  LPSTR   Match           ,
                            OUT LPSTR * FileNameStop    ,
                            OUT LPSTR * MatchStop        )
    {
        return ( ::SymMatchFileName ( FileName       ,
                                      Match          ,
                                      FileNameStop   ,
                                      MatchStop       ) ) ;
    }

/*----------------------------------------------------------------------
                          Public Miscellaneous Members
----------------------------------------------------------------------*/
public      :

    LPVOID SymFunctionTableAccess ( DWORD AddrBase )
    {
        return ( ::SymFunctionTableAccess ( m_hProcess , AddrBase ) ) ;
    }

    BOOL SymGetSearchPath ( OUT LPSTR SearchPath        ,
                            IN  DWORD SearchPathLength   )
    {
        return ( ::SymGetSearchPath ( m_hProcess       ,
                                      SearchPath       ,
                                      SearchPathLength  ) ) ;
    }

    BOOL SymSetSearchPath ( IN LPSTR SearchPath )
    {
        return ( ::SymSetSearchPath ( m_hProcess , SearchPath ) ) ;
    }

    BOOL SymRegisterCallback ( IN PSYMBOL_REGISTERED_CALLBACK
                                                       CallbackFunction,
                               IN PVOID                UserContext    )
    {
        return ( ::SymRegisterCallback ( m_hProcess         ,
                                         CallbackFunction   ,
                                         UserContext         ) ) ;
    }


/*----------------------------------------------------------------------
                         Protected Data Members
----------------------------------------------------------------------*/
protected   :
    // The unique value that will be used for this instance of the
    // symbol engine. This value doesn’t have to be an actual
    // process value, just a unique value.
    HANDLE      m_hProcess      ;

} ;

#endif      // _SYMBOLENGINE_H
