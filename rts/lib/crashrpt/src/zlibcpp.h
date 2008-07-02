///////////////////////////////////////////////////////////////////////////////
//
//  Module: zlibcpp.h
//
//    Desc: Basic class wrapper for the zlib dll
//
// Copyright (c) 2003 Automatic Data Processing, Inc. All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ZLIBCPP_H_
#define _ZLIBCPP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef ZLIB_DLL
#define ZLIB_DLL
#endif // !ZLIB_DLL

#ifndef _WINDOWS
#define _WINDOWS
#endif // !_WINDOWS

#ifndef _zip_H
#include "../zlib/include/zip.h"
#pragma comment(lib, "../zlib/lib/zlib.lib")
#endif // _zip_H

#include <atlmisc.h>

class CZLib  
{
public:
	CZLib();
	virtual ~CZLib();

	BOOL Open(CString f_file, int f_nAppend = 0);
   BOOL AddFile(CString f_file);
	void Close();
protected:
	zipFile m_zf;
};

#endif // !_ZLIBCPP_H
