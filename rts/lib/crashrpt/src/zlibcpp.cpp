///////////////////////////////////////////////////////////////////////////////
//
//  Module: zlibcpp.cpp
//
//    Desc: See zlibcpp.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "zlibcpp.h"
#include "utility.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
// CZLib::CZLib
//
// 
//
CZLib::CZLib()
{
   m_zf = 0;
}


//-----------------------------------------------------------------------------
// CZLib::~CZLib
//
// Close open zip file
//
CZLib::~CZLib()
{
   if (m_zf)
      Close();
}


//-----------------------------------------------------------------------------
// CZLib::Open
//
// Create or open zip file
//
BOOL CZLib::Open(CString f_file, int f_nAppend)
{
   m_zf = zipOpen(f_file, f_nAppend);
   return (m_zf != NULL);
}


//-----------------------------------------------------------------------------
// CZLib::Close
//
// Close open zip file
//
void CZLib::Close()
{
   if (m_zf)
      zipClose(m_zf, NULL);

   m_zf = 0;
}


//-----------------------------------------------------------------------------
// CZLib::AddFile
//
// Adds a file to the zip archive
//
BOOL CZLib::AddFile(CString f_file)
{
   BOOL bReturn = FALSE;

   // Open file being added
   HANDLE hFile = NULL;
   hFile = CreateFile(f_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile)
   {
      // Get file creation date
      FILETIME       ft = CUtility::getLastWriteFileTime(f_file);
      zip_fileinfo   zi = {0};

      FileTimeToDosDateTime(
         &ft,                       // last write FILETIME
         ((LPWORD)&zi.dosDate)+1,   // dos date
         ((LPWORD)&zi.dosDate)+0);  // dos time

      // Trim path off file name
      CString sFileName = f_file.Mid(f_file.ReverseFind(_T('\\')) + 1);

      // Start a new file in Zip
      if (ZIP_OK == zipOpenNewFileInZip(m_zf, 
                                        sFileName, 
                                        &zi, 
                                        NULL, 
                                        0, 
                                        NULL, 
                                        0, 
                                        NULL, 
                                        Z_DEFLATED, 
                                        Z_BEST_COMPRESSION))
      {
         // Write file to Zip in 4 KB chunks 
         const DWORD BUFFSIZE    = 4096;
         TCHAR buffer[BUFFSIZE]  = _T("");
         DWORD dwBytesRead       = 0;

         while (ReadFile(hFile, &buffer, BUFFSIZE, &dwBytesRead, NULL)
                && dwBytesRead)
         {
            if (ZIP_OK == zipWriteInFileInZip(m_zf, buffer, dwBytesRead)
               && dwBytesRead < BUFFSIZE)
            {
               // Success
               bReturn = TRUE;
            }
         }

         bReturn &= (ZIP_OK == zipCloseFileInZip(m_zf));
      }
      
      bReturn &= CloseHandle(hFile);
   }

   return bReturn;
}