///////////////////////////////////////////////////////////////////////////////
//
//  Module: detaildlg.h
//
//    Desc: Dialog class used to display and preview the report contents.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DETAILDLG_H_
#define _DETAILDLG_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"
#include "aboutdlg.h"

// Defines list control column attributes
LVCOLUMN _ListColumns[] =
{
   /*
   {
      mask,
      fmt,
      cx,
      pszText,
      cchTextMax,
      iSubItem,
      iImage,
      iOrder
   }
   */
   {  // Column 1: File name
      LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH,
      LVCFMT_LEFT, 
      114, 
      (LPTSTR)IDS_NAME, 
      0, 
      0, 
      0, 
      0
   },
   {  // Column 2: File description
      LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH, 
      LVCFMT_LEFT, 
      150, 
      (LPTSTR)IDS_DESC, 
      0, 
      1, 
      0, 
      1
   },
   {  // Column 3: File type
      LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH, LVCFMT_LEFT, 
      100, 
      (LPTSTR)IDS_TYPE, 
      0, 
      2, 
      0, 
      2
   },
   {  // Column 4: File size
      LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH, 
      LVCFMT_RIGHT, 
      100, 
      (LPTSTR)IDS_SIZE, 
      0, 
      3, 
      0, 
      3
   },
};


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CDetailDlg
// 
// See the module comment at top of file.
//
class CDetailDlg : public CDialogImpl<CDetailDlg>
{
public:
	enum { IDD = IDD_DETAILDLG };

   TStrStrVector  *m_pUDFiles;      // File <name,desc>
   CImageList  m_iconList;       // Shell icon list
   
	BEGIN_MSG_MAP(CDetailDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColor)
      MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
      NOTIFY_HANDLER(IDC_FILE_LIST, LVN_ITEMCHANGED, OnItemChanged)
      NOTIFY_HANDLER(IDC_FILE_LIST, NM_DBLCLK, OnItemDblClicked)
		COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnOK)
	END_MSG_MAP()

   //-----------------------------------------------------------------------------
   // ~CDetailDlg
   //
   // 
   //
   ~CDetailDlg() 
   {
      // Release shell icon list
      m_iconList.Detach();
   };

	
   //-----------------------------------------------------------------------------
   // OnInitDialog
   //
   // 
   //
   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
      int i = 0;

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

      // center the dialog on the screen
		CenterWindow();

      CListViewCtrl list;
      list.Attach(GetDlgItem(IDC_FILE_LIST));

      // Turn on full row select
      ListView_SetExtendedListViewStyle(list.m_hWnd, LVS_EX_FULLROWSELECT);

      //
      // Attach the system image list to the list control.
      //
      SHFILEINFO sfi = {0};

      HIMAGELIST hil = (HIMAGELIST)SHGetFileInfo(
                                    NULL,
                                    0,
                                    &sfi,
                                    sizeof(sfi),
                                    SHGFI_SYSICONINDEX | SHGFI_SMALLICON);

      if (NULL != hil)
      {
         m_iconList.Attach(hil);
         list.SetImageList(m_iconList, LVSIL_SMALL);
      }

      //
      // Add column headings
      //
      for (i = 0; i < sizeof(_ListColumns) / sizeof(LVCOLUMN); i++)
      {
         list.InsertColumn(
            i, 
            CString(_ListColumns[i].pszText), 
            _ListColumns[i].fmt, 
            _ListColumns[i].cx, 
            _ListColumns[i].iSubItem);
      }

      //
      // Insert items
      //
	  WIN32_FILE_ATTRIBUTE_DATA infoData;
	  char *pfn;
      CString           sSize;
      LVITEM            lvi            = {0};
      TStrStrVector::iterator p;
	  int status;
	  char path[_MAX_PATH];
      for (i = 0, p = m_pUDFiles->begin(); p != m_pUDFiles->end(); p++, i++)
      {
		 // note that the code depends on the index (iItem) being the same as
		 // the index into m_pUDFiles, so it is not practical to ignore missing
		 // files

		 // translate supplied path to full path (SHGetFileInfo doesn't like
		 // some path constructions, such ones with \..\ in them).
		 GetFullPathName(p->first, sizeof path, path, &pfn);
         status = SHGetFileInfo(
            path,
            0,
            &sfi,
            sizeof(sfi),
            SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_TYPENAME | SHGFI_SMALLICON);
		 if (status) {
			 // Name
			 lvi.mask          = LVIF_IMAGE | LVIF_TEXT;
			 lvi.iItem         = i;
			 lvi.iSubItem      = 0;
			 lvi.iImage        = sfi.iIcon;
			 lvi.pszText       = sfi.szDisplayName;
			 list.InsertItem(&lvi);
         } else {
			 // Name
			 lvi.mask          = LVIF_IMAGE | LVIF_TEXT;
			 lvi.iItem         = i;
			 lvi.iSubItem      = 0;
			 lvi.iImage        = sfi.iIcon; /* no icon */
			 lvi.pszText       = const_cast<char *>(static_cast<const char *>(p->first));
			 list.InsertItem(&lvi);
		 }
         // Description
         list.SetItemText(i, 1, p->second);

         // Type
		 if (status) {
            list.SetItemText(i, 2, sfi.szTypeName);
		 }

         // Size
		 if (GetFileAttributesEx(path, GetFileExInfoStandard, &infoData)) {
			double fSize = infoData.nFileSizeLow / 1024.0;
			//sSize.Format(TEXT("%f bytes"), fSize);
			sSize.Format(TEXT("%d bytes"), infoData.nFileSizeLow);
            list.SetItemText(i, 3, sSize);
		 }
      }

      // Select first file
      ListView_SetItemState(
         GetDlgItem(IDC_FILE_LIST), 
         0, 
         LVIS_SELECTED, 
         LVIS_SELECTED);

      list.Detach();

      return TRUE;
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
   // OnCtlColor
   //
   // Force white background for read-only rich edit control
   //
   LRESULT OnCtlColor(UINT, WPARAM, LPARAM lParam, BOOL& bHandled)
   {
      LRESULT res = 0;
      if ((HWND)lParam == GetDlgItem(IDC_FILE_EDIT))
         res = (LRESULT)GetSysColorBrush(COLOR_WINDOW);

      bHandled = TRUE;

      return res;
   }

	
   //-----------------------------------------------------------------------------
   // OnOK
   //
   // 
   //
   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}

   
   //-----------------------------------------------------------------------------
   // OnItemChanged
   //
   // Update file preview
   //
   LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      LPNMLISTVIEW lpItem           = (LPNMLISTVIEW)pnmh; 
      int iItem                     = lpItem->iItem;

      if (lpItem->uChanged & LVIF_STATE
         && lpItem->uNewState & LVIS_SELECTED)
      {
         SelectItem(iItem);
      }
   
      return 0;
   }

   
   //-----------------------------------------------------------------------------
   // OnItemDblClicked
   //
   // Open file in associated application
   //
   LRESULT OnItemDblClicked(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      LPNMLISTVIEW lpItem           = (LPNMLISTVIEW)pnmh; 
      int iItem                     = lpItem->iItem;
      DWORD_PTR dwRet               = 0;

      if (iItem < 0 || (int)m_pUDFiles->size() < iItem)
         return 0;


      dwRet = (DWORD_PTR)::ShellExecute(
                              m_hWnd,
                              _T("open"),
                              m_pUDFiles->at(iItem).first,
                              NULL,    // parameters (if a program)
                              NULL,    // default directory
                              SW_SHOWNORMAL
                              );
      ATLASSERT(dwRet > 32);

      return 0;
   }

   
   //-----------------------------------------------------------------------------
   // SelectItem
   //
   // Does the work of opening the file and displaying it in the preview edit control.
   //
   void SelectItem(int iItem)
   {
      const int MAX_FILE_SIZE          = 32768; // 32k file preview max
      DWORD dwBytesRead                = 0;
      TCHAR buffer[MAX_FILE_SIZE + 1]  = _T("");

      // Sanity check
      if (iItem < 0 || (int)m_pUDFiles->size() < iItem)
          return;
 

      // 
      // Update preview header info
      //
      ::SetWindowText(GetDlgItem(IDC_NAME), m_pUDFiles->at(iItem).first);
      ::SetWindowText(GetDlgItem(IDC_DESCRIPTION), m_pUDFiles->at(iItem).second);

      //
      // Display file contents in preview window
      //
      HANDLE hFile = CreateFile(
         m_pUDFiles->at(iItem).first,
         GENERIC_READ,
         FILE_SHARE_READ | FILE_SHARE_WRITE,
         NULL,
         OPEN_EXISTING,
         FILE_ATTRIBUTE_NORMAL,
         0);

      if (NULL != hFile)
      {
         // Read up to first 32 KB
         ReadFile(hFile, buffer, MAX_FILE_SIZE, &dwBytesRead, 0);
         buffer[dwBytesRead] = 0;
         CloseHandle(hFile);
      }

      // Update edit control with file contents
      ::SetWindowText(GetDlgItem(IDC_FILE_EDIT), buffer);
   }
};

#endif	// #ifndef _DETAILDLG_H_
