///////////////////////////////////////////////////////////////////////////////
//
//  Module: aboutdlg.h
//
//    Desc: About dialog
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ABOUTDLG_H_
#define _ABOUTDLG_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "DeadLink.h"
#include "MailMsg.h"

////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CAboutDlg
// 
// See the module comment at top of file.
//
class CAboutDlg : public CDialogImpl<CAboutDlg>
{
public:
	enum { IDD = IDD_ABOUTBOX };

   CDeadLink m_license;
   CDeadLink m_email;

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_HANDLER(IDC_EMAIL_LINK, DL_CLICKED, OnEmailClick)
      COMMAND_HANDLER(IDC_LICENSE_LINK, DL_CLICKED, OnLicenseClick)
		COMMAND_ID_HANDLER(IDOK, OnOk)
	END_MSG_MAP()

   
   //-----------------------------------------------------------------------------
   // OnInitDialog
   //
   // 
   //
   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();

      //
      // Hook license link
      //
      m_license.SubclassWindow(GetDlgItem(IDC_LICENSE_LINK));   

      //
      // Hook email link
      //
      m_email.SubclassWindow(GetDlgItem(IDC_EMAIL_LINK));


      return TRUE;
	}

	LRESULT OnEmailClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
      CMailMsg msg;
      msg
         .SetTo(_T("mcarruth@email.com"))
         .SetSubject(_T("Crash Report Feedback"))
         .Send();

      return 0;
   }

   LRESULT OnLicenseClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
      HRSRC hrsrc = NULL;
      HGLOBAL hgres = NULL;

      hrsrc = FindResource(GetModuleHandle(_T("crashrpt.dll")), MAKEINTRESOURCE(IDR_LICENSE), "TEXT");

      hgres = LoadResource(GetModuleHandle(_T("crashrpt.dll")), hrsrc);

      char *pText = (char*)hgres;

      MessageBox(pText);

      return 0;
   }

   LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
      EndDialog(wID);

      return 0;
   }
  
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif	// #ifndef _ABOUTDLG_H_
