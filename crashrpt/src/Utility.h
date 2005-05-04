///////////////////////////////////////////////////////////////////////////////
//
//  Module: Utility.h
//
//    Desc: Misc static helper methods
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _UTILITY_H_
#define _UTILITY_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "stdafx.h"
#include <atlmisc.h> // CString


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CUtility
// 
// See the module comment at top of file.
//
namespace CUtility 
{

   //-----------------------------------------------------------------------------
   // getLastWriteFileTime
   //    Returns the time the file was last modified in a FILETIME structure.
   //
   // Parameters
   //    sFile       Fully qualified file name
   //
   // Return Values
   //    FILETIME structure
   //
   // Remarks
   //
   FILETIME 
   getLastWriteFileTime(
      CString sFile
      );
   
   //-----------------------------------------------------------------------------
   // getAppName
   //    Returns the application module's file name
   //
   // Parameters
   //    none
   //
   // Return Values
   //    File name of the executable
   //
   // Remarks
   //    none
   //
   CString 
   getAppName();

   //-----------------------------------------------------------------------------
   // getSaveFileName
   //    Presents the user with a save as dialog and returns the name selected.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    Name of the file to save to, or "" if the user cancels.
   //
   // Remarks
   //    none
   //
   CString 
   getSaveFileName();
	
   //-----------------------------------------------------------------------------
   // getTempFileName
   //    Returns a generated temporary file name
   //
   // Parameters
   //    none
   //
   // Return Values
   //    Temporary file name
   //
   // Remarks
   //
   CString 
   getTempFileName();
};

#endif	// #ifndef _UTILITY_H_
