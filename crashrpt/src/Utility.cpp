///////////////////////////////////////////////////////////////////////////////
//
//  Module: Utility.cpp
//
//    Desc: See Utility.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Utility.h"
#include "atldlgs.h"
#include "resource.h"

namespace CUtility {

FILETIME CUtility::getLastWriteFileTime(CString sFile)
{
   FILETIME          ftLocal = {0};
   HANDLE            hFind;
   WIN32_FIND_DATA   ff32;
   hFind = FindFirstFile(sFile, &ff32);
   if (INVALID_HANDLE_VALUE != hFind)
   {
      FileTimeToLocalFileTime(&(ff32.ftLastWriteTime), &ftLocal);
      FindClose(hFind);        
   }
   return ftLocal;
}


CString CUtility::getAppName()
{
   TCHAR szFileName[_MAX_PATH];
   GetModuleFileName(NULL, szFileName, _MAX_FNAME);

   CString sAppName; // Extract from last '\' to '.'
   sAppName = szFileName;
   sAppName = sAppName.Mid(sAppName.ReverseFind(_T('\\')) + 1)
                      .SpanExcluding(_T("."));

   return sAppName;
}


CString CUtility::getTempFileName()
{
   static int counter = 0;
   TCHAR szTempDir[MAX_PATH - 14]   = _T("");
   TCHAR szTempFile[MAX_PATH]       = _T("");

   if (GetTempPath(MAX_PATH - 14, szTempDir))
      GetTempFileName(szTempDir, getAppName(), ++counter, szTempFile);

   return szTempFile;
}


CString CUtility::getSaveFileName()
{
   CString sFilter((LPCTSTR)IDS_ZIP_FILTER);

   CFileDialog fd(
      FALSE, 
      _T("zip"), 
      getAppName(), 
      OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
      sFilter);

	if (IDOK == fd.DoModal())
   {
      DeleteFile(fd.m_szFileName);  // Just in-case it already exist
      return fd.m_szFileName;
   }

   return _T("");
}

};
