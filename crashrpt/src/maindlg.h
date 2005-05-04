///////////////////////////////////////////////////////////////////////////////
//
//  Module: maindlg.h
//
//    Desc: Main crash report dialog, responsible for gathering additional
//          user information and allowing user to examine crash report.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAINDLG_H_
#define _MAINDLG_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Utility.h"
#include "DeadLink.h"
#include "detaildlg.h"
#include "aboutdlg.h"

//
// RTF load callback
//
DWORD CALLBACK LoadRTFString(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   CString *sText = (CString*)dwCookie;
   LONG lLen = sText->GetLength();

   for (*pcb = 0; *pcb < cb && *pcb < lLen; (*pcb)++)
   {  
      pbBuff[*pcb] = sText->GetAt(*pcb);
   }

   return 0;
}


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CMainDlg
// 
// See the module comment at top of file.
//
class CMainDlg : public CDialogImpl<CMainDlg>
{
public:
	enum { IDD = IDD_MAINDLG };

   CString     m_sEmail;         // Email: From
   CString     m_sDescription;   // Email: Body
   CDeadLink   m_link;           // Dead link
   TStrStrVector  *m_pUDFiles;      // Files <name,desc>
   BOOL        m_sendButton;     // Display 'Send' or 'Save' button

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDC_LINK, OnLinkClick)
      MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
		COMMAND_ID_HANDLER(IDOK, OnSend)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_SAVE, OnCancel)
		COMMAND_ID_HANDLER(IDC_DEBUG, OnCancel)
	END_MSG_MAP()

   
   //-----------------------------------------------------------------------------
   // CMainDlg
   //
   // Loads RichEditCtrl library
   //
   CMainDlg() 
   {
      LoadLibrary(CRichEditCtrl::GetLibraryName());
   };

	
   //-----------------------------------------------------------------------------
   // OnInitDialog
   //
   // 
   //
   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();

	   // Add "About..." menu item to system menu.

	   // IDM_ABOUTBOX must be in the system command range.
      ATLASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX); 
      ATLASSERT(IDM_ABOUTBOX < 0xF000); 

      CMenu sysMenu;
      sysMenu.Attach(GetSystemMenu(FALSE));
//      if (sysMenu.IsMenu())
      {
		   CString strAboutMenu;
		   strAboutMenu.LoadString(IDS_ABOUTBOX);
		   if (!strAboutMenu.IsEmpty())
		   {
            sysMenu.AppendMenu(MF_SEPARATOR);
			   sysMenu.AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		   }
	   }
      //
      // Set title using app name
      //
      SetWindowText(CUtility::getAppName());

	  // Hide 'Send' button if required. Position 'Send' and 'Save'.
	  //
	  HWND okButton = GetDlgItem(IDOK);
      HWND saveButton = GetDlgItem(IDC_SAVE);
	  if (m_sendButton) {
		// Line up Save, Send [OK] and Exit [Cancel] all in a row
		HWND cancelButton = GetDlgItem(IDCANCEL);
		WINDOWPLACEMENT okPlace;
		WINDOWPLACEMENT savePlace;
		WINDOWPLACEMENT cancelPlace;

		::GetWindowPlacement(okButton, &okPlace);
		::GetWindowPlacement(saveButton, &savePlace);
		::GetWindowPlacement(cancelButton, &cancelPlace);

		savePlace.rcNormalPosition.left =
			okPlace.rcNormalPosition.left -
			(savePlace.rcNormalPosition.right - savePlace.rcNormalPosition.left) +
			(okPlace.rcNormalPosition.right - cancelPlace.rcNormalPosition.left);
		::SetWindowPlacement(saveButton, &savePlace);

		DWORD style = ::GetWindowLong(okButton, GWL_STYLE);
		::SetWindowLong(okButton, GWL_STYLE, style  | WS_VISIBLE);
	  } else {
		WINDOWPLACEMENT okPlace;

		// Put Save on top of the invisible Send [OK]
		::GetWindowPlacement(okButton, &okPlace);

		::SetWindowPlacement(saveButton, &okPlace);

		DWORD style = ::GetWindowLong(okButton, GWL_STYLE);
		::SetWindowLong(okButton, GWL_STYLE, style  & ~ WS_VISIBLE);
	  }
	  // Enable Debug button if appropriate
	  HKEY key;
	  if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Carruth\\CrashRpt", 0, KEY_READ, &key) == ERROR_SUCCESS) {
		  DWORD type;
		  DWORD data;
		  DWORD data_len = sizeof data;
		  if (RegQueryValueEx(key, "EnableDebug", 0, &type, reinterpret_cast<LPBYTE>(&data), &data_len) == ERROR_SUCCESS &&
			  type == REG_DWORD) {
			  HWND debugButton = GetDlgItem(IDC_DEBUG);
			  DWORD style = ::GetWindowLong(debugButton, GWL_STYLE);
			  ::SetWindowLong(debugButton, GWL_STYLE, style | WS_VISIBLE);
		  }
		  RegCloseKey(key);
	  }

      //
      // Use app icon
      //
      CStatic icon;
      icon.Attach(GetDlgItem(IDI_APPICON));
      icon.SetIcon(::LoadIcon((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINFRAME)));
      icon.Detach();

      //
      // Set failure heading
      //
      EDITSTREAM es;
      es.pfnCallback = LoadRTFString;

      CString sText;
      sText.Format(IDS_HEADER, CUtility::getAppName());
      es.dwCookie = (DWORD)&sText;

      CRichEditCtrl re;
      re.Attach(GetDlgItem(IDC_HEADING_TEXT));
      re.StreamIn(SF_RTF, es);
      re.Detach();

	  static char username[_MAX_PATH];
	  DWORD size = sizeof username;
	  if (::GetUserName(username, &size)) {
	      HWND     hWndEmail = GetDlgItem(IDC_EMAIL);
		  ::SetWindowText(hWndEmail, username);
	  }

      //
      // Hook dead link
      //
      m_link.SubclassWindow(GetDlgItem(IDC_LINK));   

      return TRUE;
	}

	
   //-----------------------------------------------------------------------------
   // OnLinkClick
   //
   // Display details dialog instead of opening URL
   //
   LRESULT OnLinkClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CDetailDlg dlg;
      dlg.m_pUDFiles = m_pUDFiles;
      dlg.DoModal();
      return 0;
   }

   //-----------------------------------------------------------------------------
   // OnSysCommand
   //
   // 
   //
   LRESULT OnSysCommand(UINT, WPARAM wParam, LPARAM , BOOL& bHandled)
   {
      bHandled = FALSE;

      if ((wParam & 0xFFF0) == IDM_ABOUTBOX)
      {
         CAboutDlg dlg;
         dlg.DoModal();
         bHandled = TRUE;
      }

      return 0;
   }

	
   //-----------------------------------------------------------------------------
   // OnSend
   //
   // Send handler, validates email address entered, if one, and returns.
   //
   LRESULT OnSend(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
      HWND     hWndEmail = GetDlgItem(IDC_EMAIL);
      HWND     hWndDesc = GetDlgItem(IDC_DESCRIPTION);
	   int      nEmailLen = ::GetWindowTextLength(hWndEmail) + 1;
      int      nDescLen = ::GetWindowTextLength(hWndDesc) + 1;

      LPTSTR lpStr = m_sEmail.GetBufferSetLength(nEmailLen);
      ::GetWindowText(hWndEmail, lpStr, nEmailLen);
      m_sEmail.ReleaseBuffer();

      lpStr = m_sDescription.GetBufferSetLength(nDescLen);
      ::GetWindowText(hWndDesc, lpStr, nDescLen);
      m_sDescription.ReleaseBuffer();

      //
      // If an email address was entered, verify that
      // it [1] contains a @ and [2] the last . comes
      // after the @.
      //
      if (m_sEmail.GetLength() &&
          (m_sEmail.Find(_T('@')) < 0 ||
           m_sEmail.ReverseFind(_T('.')) < m_sEmail.Find(_T('@'))))
      {
         // alert user
         TCHAR szBuf[256];
		   ::LoadString(_Module.GetResourceInstance(), IDS_INVALID_EMAIL, szBuf, 255);
         MessageBox(szBuf, CUtility::getAppName(), MB_OK);
         // select email
         ::SetFocus(hWndEmail);
      }
      else
      {
         EndDialog(wID);
      }

      return 0;
   }

   //-----------------------------------------------------------------------------
   // OnCancel
   //
   // 
   //
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
      EndDialog(wID);
		return 0;
   }

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif	// #ifndef _MAINDLG_H_
