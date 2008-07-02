// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__465AD6C5_1ACE_47ED_AD54_7ED140DFF7CC__INCLUDED_)
#define AFX_STDAFX_H__465AD6C5_1ACE_47ED_AD54_7ED140DFF7CC__INCLUDED_

// Change these values to use different versions
#define WINVER		0x0400
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400
#define _RICHEDIT_VER	0x0100

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

#define CRASHRPTAPI extern "C" __declspec(dllexport)
#ifndef _DEBUG
// 4189: unused variable
// 4530: unwind semantics not enabled
#pragma warning(disable: 4189 4530)
#endif
#define chSTR2(x) #x
#define chSTR(x) chSTR2(x)
#define chMSG(desc) message(__FILE__ "(" chSTR(__LINE__) "):" #desc)
#define todo(desc) message(__FILE__ "(" chSTR(__LINE__) "): TODO: " #desc)
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__465AD6C5_1ACE_47ED_AD54_7ED140DFF7CC__INCLUDED_)
