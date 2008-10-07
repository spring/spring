// Windows Template Library - WTL version 7.0
// Copyright (C) 1997-2002 Microsoft Corporation
// All rights reserved.
//
// This file is a part of the Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.

#ifndef __ATLDLGS_H__
#define __ATLDLGS_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
	#error atldlgs.h requires atlapp.h to be included first
#endif

#ifndef __ATLWIN_H__
	#error atldlgs.h requires atlwin.h to be included first
#endif

#include <commdlg.h>
#include <shlobj.h>


/////////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CFileDialogImpl<T>
// CFileDialog
// CFolderDialogImpl<T>
// CFolderDialog
// CFontDialogImpl<T>
// CFontDialog
// CRichEditFontDialogImpl<T>
// CRichEditFontDialog
// CColorDialogImpl<T>
// CColorDialog
// CPrintDialogImpl<T>
// CPrintDialog
// CPageSetupDialogImpl<T>
// CPrintDialogExImpl<T>
// CPrintDialogEx
// CPageSetupDialog
// CFindReplaceDialogImpl<T>
// CFindReplaceDialog
//
// CPropertySheetWindow
// CPropertySheetImpl<T, TBase>
// CPropertySheet
// CPropertyPageWindow
// CPropertyPageImpl<T, TBase>
// CPropertyPage<t_wDlgTemplateID>


namespace WTL
{

/////////////////////////////////////////////////////////////////////////////
// CFileDialogImpl - used for File Open or File Save As

// compatibility with the old (vc6.0) headers
#if (_WIN32_WINNT >= 0x0500) && !defined(OPENFILENAME_SIZE_VERSION_400)
#ifndef CDSIZEOF_STRUCT
#define CDSIZEOF_STRUCT(structname, member)  (((int)((LPBYTE)(&((structname*)0)->member) - ((LPBYTE)((structname*)0)))) + sizeof(((structname*)0)->member))
#endif
#define OPENFILENAME_SIZE_VERSION_400A  CDSIZEOF_STRUCT(OPENFILENAMEA,lpTemplateName)
#define OPENFILENAME_SIZE_VERSION_400W  CDSIZEOF_STRUCT(OPENFILENAMEW,lpTemplateName)
#ifdef UNICODE
#define OPENFILENAME_SIZE_VERSION_400  OPENFILENAME_SIZE_VERSION_400W
#else
#define OPENFILENAME_SIZE_VERSION_400  OPENFILENAME_SIZE_VERSION_400A
#endif // !UNICODE
#endif // (_WIN32_WINNT >= 0x0500) && !defined(OPENFILENAME_SIZE_VERSION_400)

template <class T>
class ATL_NO_VTABLE CFileDialogImpl : public CDialogImplBase
{
public:
	OPENFILENAME m_ofn;
	BOOL m_bOpenFileDialog;			// TRUE for file open, FALSE for file save
	TCHAR m_szFileTitle[_MAX_FNAME];	// contains file title after return
	TCHAR m_szFileName[_MAX_PATH];		// contains full path name after return

	CFileDialogImpl(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
			LPCTSTR lpszDefExt = NULL,
			LPCTSTR lpszFileName = NULL,
			DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			LPCTSTR lpszFilter = NULL,
			HWND hWndParent = NULL)
	{
		memset(&m_ofn, 0, sizeof(m_ofn)); // initialize structure to 0/NULL
		m_szFileName[0] = '\0';
		m_szFileTitle[0] = '\0';

		m_bOpenFileDialog = bOpenFileDialog;

		m_ofn.lStructSize = sizeof(m_ofn);
#if (_WIN32_WINNT >= 0x0500)
		// adjust struct size if running on older version of Windows
		if(AtlIsOldWindows())
		{
			ATLASSERT(sizeof(m_ofn) > OPENFILENAME_SIZE_VERSION_400);	// must be
			m_ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
		}
#endif //(_WIN32_WINNT >= 0x0500)
		m_ofn.lpstrFile = m_szFileName;
		m_ofn.nMaxFile = _MAX_PATH;
		m_ofn.lpstrDefExt = lpszDefExt;
		m_ofn.lpstrFileTitle = (LPTSTR)m_szFileTitle;
		m_ofn.nMaxFileTitle = _MAX_FNAME;
		m_ofn.Flags |= dwFlags | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING;
		m_ofn.lpstrFilter = lpszFilter;
		m_ofn.hInstance = _Module.GetResourceInstance();
		m_ofn.lpfnHook = (LPOFNHOOKPROC)T::StartDialogProc;
		m_ofn.hwndOwner = hWndParent;

		// setup initial file name
		if(lpszFileName != NULL)
			lstrcpyn(m_szFileName, lpszFileName, _MAX_PATH);
	}

	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_ofn.Flags & OFN_ENABLEHOOK);
		ATLASSERT(m_ofn.lpfnHook != NULL);	// can still be a user hook

		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		if(m_ofn.hwndOwner == NULL)		// set only if not specified before
			m_ofn.hwndOwner = hWndParent;

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CDialogImplBase*)this);

		BOOL bRet;
		if(m_bOpenFileDialog)
			bRet = ::GetOpenFileName(&m_ofn);
		else
			bRet = ::GetSaveFileName(&m_ofn);

		m_hWnd = NULL;

		return bRet ? IDOK : IDCANCEL;
	}

// Attributes
	CWindow GetFileDialogWindow() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(GetParent());
	}

	int GetFilePath(LPTSTR lpstrFilePath, int nLength) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		return (int)GetFileDialogWindow().SendMessage(CDM_GETFILEPATH, nLength, (LPARAM)lpstrFilePath);
	}

	int GetFolderIDList(LPVOID lpBuff, int nLength) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		return (int)GetFileDialogWindow().SendMessage(CDM_GETFOLDERIDLIST, nLength, (LPARAM)lpBuff);
	}

	int GetFolderPath(LPTSTR lpstrFolderPath, int nLength) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		return (int)GetFileDialogWindow().SendMessage(CDM_GETFOLDERPATH, nLength, (LPARAM)lpstrFolderPath);
	}

	int GetSpec(LPTSTR lpstrSpec, int nLength) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		return (int)GetFileDialogWindow().SendMessage(CDM_GETSPEC, nLength, (LPARAM)lpstrSpec);
	}

	void SetControlText(int nCtrlID, LPCTSTR lpstrText)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		GetFileDialogWindow().SendMessage(CDM_SETCONTROLTEXT, nCtrlID, (LPARAM)lpstrText);
	}

	void SetDefExt(LPCTSTR lpstrExt)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		GetFileDialogWindow().SendMessage(CDM_SETDEFEXT, 0, (LPARAM)lpstrExt);
	}

	BOOL GetReadOnlyPref() const	// return TRUE if readonly checked
	{
		return (m_ofn.Flags & OFN_READONLY) ? TRUE : FALSE;
	}

// Operations
	void HideControl(int nCtrlID)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		GetFileDialogWindow().SendMessage(CDM_HIDECONTROL, nCtrlID);
	}

// Special override for common dialogs
	BOOL EndDialog(INT_PTR /*nRetCode*/ = 0)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		GetFileDialogWindow().SendMessage(WM_COMMAND, MAKEWPARAM(IDCANCEL, 0));
		return TRUE;
	}

// Message map and handlers
	BEGIN_MSG_MAP(CFileDialogImpl)
		NOTIFY_CODE_HANDLER(CDN_FILEOK, _OnFileOK)
		NOTIFY_CODE_HANDLER(CDN_FOLDERCHANGE, _OnFolderChange)
		NOTIFY_CODE_HANDLER(CDN_HELP, _OnHelp)
		NOTIFY_CODE_HANDLER(CDN_INITDONE, _OnInitDone)
		NOTIFY_CODE_HANDLER(CDN_SELCHANGE, _OnSelChange)
		NOTIFY_CODE_HANDLER(CDN_SHAREVIOLATION, _OnShareViolation)
		NOTIFY_CODE_HANDLER(CDN_TYPECHANGE, _OnTypeChange)
	END_MSG_MAP()

	LRESULT _OnFileOK(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		return !pT->OnFileOK((LPOFNOTIFY)pnmh);
	}
	LRESULT _OnFolderChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT->OnFolderChange((LPOFNOTIFY)pnmh);
		return 0;
	}
	LRESULT _OnHelp(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT->OnHelp((LPOFNOTIFY)pnmh);
		return 0;
	}
	LRESULT _OnInitDone(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT->OnInitDone((LPOFNOTIFY)pnmh);
		return 0;
	}
	LRESULT _OnSelChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT->OnSelChange((LPOFNOTIFY)pnmh);
		return 0;
	}
	LRESULT _OnShareViolation(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		return pT->OnShareViolation((LPOFNOTIFY)pnmh);
	}
	LRESULT _OnTypeChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT->OnTypeChange((LPOFNOTIFY)pnmh);
		return 0;
	}

// Overrideables
	BOOL OnFileOK(LPOFNOTIFY /*lpon*/)
	{
		return TRUE;
	}
	void OnFolderChange(LPOFNOTIFY /*lpon*/)
	{
	}
	void OnHelp(LPOFNOTIFY /*lpon*/)
	{
	}
	void OnInitDone(LPOFNOTIFY /*lpon*/)
	{
	}
	void OnSelChange(LPOFNOTIFY /*lpon*/)
	{
	}
	int OnShareViolation(LPOFNOTIFY /*lpon*/)
	{
		return 0;
	}
	void OnTypeChange(LPOFNOTIFY /*lpon*/)
	{
	}
};


class CFileDialog : public CFileDialogImpl<CFileDialog>
{
public:
	CFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		HWND hWndParent = NULL)
		: CFileDialogImpl<CFileDialog>(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, hWndParent)
	{ }

	// override base class map and references to handlers
	DECLARE_EMPTY_MSG_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFolderDialogImpl - used for browsing for a folder

template <class T>
class ATL_NO_VTABLE CFolderDialogImpl
{
public:
	BROWSEINFO m_bi;
	TCHAR m_szFolderDisplayName[MAX_PATH];
	TCHAR m_szFolderPath[MAX_PATH];
	HWND m_hWnd;	// used only in the callback function

// Constructor
	CFolderDialogImpl(HWND hWndParent = NULL, LPCTSTR lpstrTitle = NULL, UINT uFlags = BIF_RETURNONLYFSDIRS)
	{
		memset(&m_bi, 0, sizeof(m_bi)); // initialize structure to 0/NULL

		m_bi.hwndOwner = hWndParent;
		m_bi.pidlRoot = NULL;
		m_bi.pszDisplayName = m_szFolderDisplayName;
		m_bi.lpszTitle = lpstrTitle;
		m_bi.ulFlags = uFlags;
		m_bi.lpfn = BrowseCallbackProc;
		m_bi.lParam = (LPARAM)static_cast<T*>(this);

		m_szFolderPath[0] = 0;
		m_szFolderDisplayName[0] = 0;

		m_hWnd = NULL;
	}

// Operations
	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		if(m_bi.hwndOwner == NULL)	// set only if not specified before
			m_bi.hwndOwner = hWndParent;

		INT_PTR nRet = -1;
		LPITEMIDLIST pItemIDList = ::SHBrowseForFolder(&m_bi);
		if(pItemIDList != NULL)
		{
			if(::SHGetPathFromIDList(pItemIDList, m_szFolderPath))
			{
				IMalloc* pMalloc = NULL;
				if(SUCCEEDED(::SHGetMalloc(&pMalloc)))
				{
					pMalloc->Free(pItemIDList);
					pMalloc->Release();
				}
				nRet = IDOK;
			}
			else
			{
				nRet = IDCANCEL;
			}
		}

		return nRet;
	}

	// filled after a call to DoModal
	LPCTSTR GetFolderPath() const
	{
		return m_szFolderPath;
	}
	LPCTSTR GetFolderDisplayName() const
	{
		return m_szFolderDisplayName;
	}
	int GetFolderImageIndex() const
	{
		return m_bi.iImage;
	}

// Callback function and overrideables
	static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
	{
#ifndef BFFM_VALIDATEFAILED
#ifdef UNICODE
		const int BFFM_VALIDATEFAILED = 4;
#else
		const int BFFM_VALIDATEFAILED = 3;
#endif
#endif //!BFFM_VALIDATEFAILED
		int nRet = 0;
		T* pT = (T*)lpData;
		pT->m_hWnd = hWnd;
		switch(uMsg)
		{
		case BFFM_INITIALIZED:
			pT->OnInitialized();
			break;
		case BFFM_SELCHANGED:
			pT->OnSelChanged((LPITEMIDLIST)lParam);
			break;
		case BFFM_VALIDATEFAILED:
			nRet = pT->OnValidateFailed((LPCTSTR)lParam);
			break;
		default:
			ATLTRACE2(atlTraceUI, 0, _T("Unknown message received in CFolderDialogImpl::BrowseCallbackProc\n"));
			break;
		}
		pT->m_hWnd = NULL;
		return nRet;
	}
	void OnInitialized()
	{
	}
	void OnSelChanged(LPITEMIDLIST /*pItemIDList*/)
	{
	}
	int OnValidateFailed(LPCTSTR /*lpstrFolderPath*/)
	{
		return 1;	// 1=continue, 0=EndDialog
	}

	// Commands - valid to call only from handlers
	void EnableOK(BOOL bEnable)
	{
		ATLASSERT(m_hWnd != NULL);
		::SendMessage(m_hWnd, BFFM_ENABLEOK, bEnable, 0L);
	}
	void SetSelection(LPITEMIDLIST pItemIDList)
	{
		ATLASSERT(m_hWnd != NULL);
		::SendMessage(m_hWnd, BFFM_SETSELECTION, FALSE, (LPARAM)pItemIDList);
	}
	void SetSelection(LPCTSTR lpstrFolderPath)
	{
		ATLASSERT(m_hWnd != NULL);
		::SendMessage(m_hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpstrFolderPath);
	}
	void SetStatusText(LPCTSTR lpstrText)
	{
		ATLASSERT(m_hWnd != NULL);
		::SendMessage(m_hWnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)lpstrText);
	}
};

class CFolderDialog : public CFolderDialogImpl<CFolderDialog>
{
public:
	CFolderDialog(HWND hWndParent = NULL, LPCTSTR lpstrTitle = NULL, UINT uFlags = BIF_RETURNONLYFSDIRS)
		: CFolderDialogImpl<CFolderDialog>(hWndParent, lpstrTitle, uFlags)
	{
		m_bi.lpfn = NULL;
	}
};


/////////////////////////////////////////////////////////////////////////////
// CCommonDialogImplBase - base class for common dialog classes

class ATL_NO_VTABLE CCommonDialogImplBase : public CWindowImplBase
{
public:
	static UINT_PTR APIENTRY HookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if(uMsg != WM_INITDIALOG)
			return 0;
		CCommonDialogImplBase* pT = (CCommonDialogImplBase*)_Module.ExtractCreateWndData();
		ATLASSERT(pT != NULL);
		ATLASSERT(pT->m_hWnd == NULL);
		ATLASSERT(::IsWindow(hWnd));
		// subclass dialog's window
		if(!pT->SubclassWindow(hWnd))
		{
			ATLTRACE2(atlTraceUI, 0, _T("Subclassing a common dialog failed\n"));
			return 0;
		}
		// check message map for WM_INITDIALOG handler
		LRESULT lRes;
		if(pT->ProcessWindowMessage(pT->m_hWnd, uMsg, wParam, lParam, lRes, 0) == FALSE)
			return 0;
		return lRes;
	}

// Special override for common dialogs
	BOOL EndDialog(INT_PTR /*nRetCode*/ = 0)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		SendMessage(WM_COMMAND, MAKEWPARAM(IDABORT, 0));
		return TRUE;
	}

// Implementation - try to override these, to prevent errors
	HWND Create(HWND, _U_RECT, LPCTSTR, DWORD, DWORD, _U_MENUorID, ATOM, LPVOID)
	{
		ATLASSERT(FALSE);	// should not be called
		return NULL;
	}
	static LRESULT CALLBACK StartWindowProc(HWND /*hWnd*/, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		ATLASSERT(FALSE);	// should not be called
		return 0;
	}
};


/////////////////////////////////////////////////////////////////////////////
// CFontDialogImpl - font selection dialog

template <class T>
class ATL_NO_VTABLE CFontDialogImpl : public CCommonDialogImplBase
{
public:
	CHOOSEFONT m_cf;
	TCHAR m_szStyleName[64];	// contains style name after return
	LOGFONT m_lf;			// default LOGFONT to store the info

// Constructors
	CFontDialogImpl(LPLOGFONT lplfInitial = NULL,
			DWORD dwFlags = CF_EFFECTS | CF_SCREENFONTS,
			HDC hDCPrinter = NULL,
			HWND hWndParent = NULL)
	{
		memset(&m_cf, 0, sizeof(m_cf));
		memset(&m_lf, 0, sizeof(m_lf));
		memset(&m_szStyleName, 0, sizeof(m_szStyleName));

		m_cf.lStructSize = sizeof(m_cf);
		m_cf.hwndOwner = hWndParent;
		m_cf.rgbColors = RGB(0, 0, 0);
		m_cf.lpszStyle = (LPTSTR)&m_szStyleName;
		m_cf.Flags = dwFlags | CF_ENABLEHOOK;
		m_cf.lpfnHook = (LPCFHOOKPROC)T::HookProc;

		if(lplfInitial != NULL)
		{
			m_cf.lpLogFont = lplfInitial;
			m_cf.Flags |= CF_INITTOLOGFONTSTRUCT;
			memcpy(&m_lf, m_cf.lpLogFont, sizeof(m_lf));
		}
		else
		{
			m_cf.lpLogFont = &m_lf;
		}

		if(hDCPrinter != NULL)
		{
			m_cf.hDC = hDCPrinter;
			m_cf.Flags |= CF_PRINTERFONTS;
		}
	}

// Operations
	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_cf.Flags & CF_ENABLEHOOK);
		ATLASSERT(m_cf.lpfnHook != NULL);	// can still be a user hook

		if(m_cf.hwndOwner == NULL)		// set only if not specified before
			m_cf.hwndOwner = hWndParent;

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)this);

		BOOL bRet = ::ChooseFont(&m_cf);

		m_hWnd = NULL;

		if(bRet)	// copy logical font from user's initialization buffer (if needed)
			memcpy(&m_lf, m_cf.lpLogFont, sizeof(m_lf));

		return bRet ? IDOK : IDCANCEL;
	}

	// Get the selected font (works during DoModal displayed or after)
	void GetCurrentFont(LPLOGFONT lplf) const
	{
		ATLASSERT(lplf != NULL);

		if(m_hWnd != NULL)
			::SendMessage(m_hWnd, WM_CHOOSEFONT_GETLOGFONT, 0, (LPARAM)lplf);
		else
			*lplf = m_lf;
	}

	// Helpers for parsing information after successful return
	LPCTSTR GetFaceName() const   // return the face name of the font
	{
		return (LPCTSTR)m_cf.lpLogFont->lfFaceName;
	}
	LPCTSTR GetStyleName() const  // return the style name of the font
	{
		return m_cf.lpszStyle;
	}
	int GetSize() const           // return the pt size of the font
	{
		return m_cf.iPointSize;
	}
	COLORREF GetColor() const     // return the color of the font
	{
		return m_cf.rgbColors;
	}
	int GetWeight() const         // return the chosen font weight
	{
		return (int)m_cf.lpLogFont->lfWeight;
	}
	BOOL IsStrikeOut() const      // return TRUE if strikeout
	{
		return (m_cf.lpLogFont->lfStrikeOut) ? TRUE : FALSE;
	}
	BOOL IsUnderline() const      // return TRUE if underline
	{
		return (m_cf.lpLogFont->lfUnderline) ? TRUE : FALSE;
	}
	BOOL IsBold() const           // return TRUE if bold font
	{
		return (m_cf.lpLogFont->lfWeight == FW_BOLD) ? TRUE : FALSE;
	}
	BOOL IsItalic() const         // return TRUE if italic font
	{
		return m_cf.lpLogFont->lfItalic ? TRUE : FALSE;
	}
};

class CFontDialog : public CFontDialogImpl<CFontDialog>
{
public:
	CFontDialog(LPLOGFONT lplfInitial = NULL,
		DWORD dwFlags = CF_EFFECTS | CF_SCREENFONTS,
		HDC hDCPrinter = NULL,
		HWND hWndParent = NULL)
		: CFontDialogImpl<CFontDialog>(lplfInitial, dwFlags, hDCPrinter, hWndParent)
	{ }

	DECLARE_EMPTY_MSG_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CRichEditFontDialogImpl - font selection for the Rich Edit ctrl

#ifdef _RICHEDIT_

template <class T>
class ATL_NO_VTABLE CRichEditFontDialogImpl : public CFontDialogImpl< T >
{
public:
	CRichEditFontDialogImpl(const CHARFORMAT& charformat,
			DWORD dwFlags = CF_SCREENFONTS,
			HDC hDCPrinter = NULL,
			HWND hWndParent = NULL)
			: CFontDialogImpl< T >(NULL, dwFlags, hDCPrinter, hWndParent)
	{
		m_cf.Flags |= CF_INITTOLOGFONTSTRUCT;
		m_cf.Flags |= FillInLogFont(charformat);
		m_cf.lpLogFont = &m_lf;

		if(charformat.dwMask & CFM_COLOR)
			m_cf.rgbColors = charformat.crTextColor;
	}

	void GetCharFormat(CHARFORMAT& cf) const
	{
		USES_CONVERSION;
		cf.dwEffects = 0;
		cf.dwMask = 0;
		if((m_cf.Flags & CF_NOSTYLESEL) == 0)
		{
			cf.dwMask |= CFM_BOLD | CFM_ITALIC;
			cf.dwEffects |= IsBold() ? CFE_BOLD : 0;
			cf.dwEffects |= IsItalic() ? CFE_ITALIC : 0;
		}
		if((m_cf.Flags & CF_NOSIZESEL) == 0)
		{
			cf.dwMask |= CFM_SIZE;
			//GetSize() returns in tenths of points so mulitply by 2 to get twips
			cf.yHeight = GetSize() * 2;
		}

		if((m_cf.Flags & CF_NOFACESEL) == 0)
		{
			cf.dwMask |= CFM_FACE;
			cf.bPitchAndFamily = m_cf.lpLogFont->lfPitchAndFamily;
#if (_RICHEDIT_VER >= 0x0200)
			lstrcpy(cf.szFaceName, GetFaceName());
#else
			lstrcpyA(cf.szFaceName, T2A((LPTSTR)(LPCTSTR)GetFaceName()));
#endif //(_RICHEDIT_VER >= 0x0200)
		}

		if(m_cf.Flags & CF_EFFECTS)
		{
			cf.dwMask |= CFM_UNDERLINE | CFM_STRIKEOUT | CFM_COLOR;
			cf.dwEffects |= IsUnderline() ? CFE_UNDERLINE : 0;
			cf.dwEffects |= IsStrikeOut() ? CFE_STRIKEOUT : 0;
			cf.crTextColor = GetColor();
		}
		if((m_cf.Flags & CF_NOSCRIPTSEL) == 0)
		{
			cf.bCharSet = m_cf.lpLogFont->lfCharSet;
			cf.dwMask |= CFM_CHARSET;
		}
		cf.yOffset = 0;
	}

	DWORD FillInLogFont(const CHARFORMAT& cf)
	{
		USES_CONVERSION;
		DWORD dwFlags = 0;
		if(cf.dwMask & CFM_SIZE)
		{
			HDC hDC = ::CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
			LONG yPerInch = ::GetDeviceCaps(hDC, LOGPIXELSY);
			m_lf.lfHeight = -(int)((cf.yHeight * yPerInch) / 1440);
		}
		else
			m_lf.lfHeight = 0;

		m_lf.lfWidth = 0;
		m_lf.lfEscapement = 0;
		m_lf.lfOrientation = 0;

		if((cf.dwMask & (CFM_ITALIC|CFM_BOLD)) == (CFM_ITALIC|CFM_BOLD))
		{
			m_lf.lfWeight = (cf.dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;
			m_lf.lfItalic = (BYTE)((cf.dwEffects & CFE_ITALIC) ? TRUE : FALSE);
		}
		else
		{
			dwFlags |= CF_NOSTYLESEL;
			m_lf.lfWeight = FW_DONTCARE;
			m_lf.lfItalic = FALSE;
		}

		if((cf.dwMask & (CFM_UNDERLINE|CFM_STRIKEOUT|CFM_COLOR)) ==
			(CFM_UNDERLINE|CFM_STRIKEOUT|CFM_COLOR))
		{
			dwFlags |= CF_EFFECTS;
			m_lf.lfUnderline = (BYTE)((cf.dwEffects & CFE_UNDERLINE) ? TRUE : FALSE);
			m_lf.lfStrikeOut = (BYTE)((cf.dwEffects & CFE_STRIKEOUT) ? TRUE : FALSE);
		}
		else
		{
			m_lf.lfUnderline = (BYTE)FALSE;
			m_lf.lfStrikeOut = (BYTE)FALSE;
		}

		if(cf.dwMask & CFM_CHARSET)
			m_lf.lfCharSet = cf.bCharSet;
		else
			dwFlags |= CF_NOSCRIPTSEL;
		m_lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
		m_lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		m_lf.lfQuality = DEFAULT_QUALITY;
		if(cf.dwMask & CFM_FACE)
		{
			m_lf.lfPitchAndFamily = cf.bPitchAndFamily;
#if (_RICHEDIT_VER >= 0x0200)
			lstrcpy(m_lf.lfFaceName, cf.szFaceName);
#else
			lstrcpy(m_lf.lfFaceName, A2T((LPSTR)cf.szFaceName));
#endif //(_RICHEDIT_VER >= 0x0200)
		}
		else
		{
			m_lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
			m_lf.lfFaceName[0] = (TCHAR)0;
		}
		return dwFlags;
	}
};

class CRichEditFontDialog : public CRichEditFontDialogImpl<CRichEditFontDialog>
{
public:
	CRichEditFontDialog(const CHARFORMAT& charformat,
		DWORD dwFlags = CF_SCREENFONTS,
		HDC hDCPrinter = NULL,
		HWND hWndParent = NULL)
		: CRichEditFontDialogImpl<CRichEditFontDialog>(charformat, dwFlags, hDCPrinter, hWndParent)
	{ }

	DECLARE_EMPTY_MSG_MAP()
};

#endif // _RICHEDIT_

/////////////////////////////////////////////////////////////////////////////
// CColorDialogImpl - color selection

template <class T>
class ATL_NO_VTABLE CColorDialogImpl : public CCommonDialogImplBase
{
public:
	CHOOSECOLOR m_cc;

// Constructor
	CColorDialogImpl(COLORREF clrInit = 0, DWORD dwFlags = 0, HWND hWndParent = NULL)
	{
		memset(&m_cc, 0, sizeof(m_cc));

		m_cc.lStructSize = sizeof(m_cc);
		m_cc.lpCustColors = GetCustomColors();
		m_cc.hwndOwner = hWndParent;
		m_cc.Flags = dwFlags | CC_ENABLEHOOK;
		m_cc.lpfnHook = (LPCCHOOKPROC)T::HookProc;

		if(clrInit != 0)
		{
			m_cc.rgbResult = clrInit;
			m_cc.Flags |= CC_RGBINIT;
		}
	}

// Operations
	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_cc.Flags & CC_ENABLEHOOK);
		ATLASSERT(m_cc.lpfnHook != NULL);	// can still be a user hook

		if(m_cc.hwndOwner == NULL)		// set only if not specified before
			m_cc.hwndOwner = hWndParent;

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)this);

		BOOL bRet = ::ChooseColor(&m_cc);

		m_hWnd = NULL;

		return bRet ? IDOK : IDCANCEL;
	}

	// Set the current color while dialog is displayed
	void SetCurrentColor(COLORREF clr)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		SendMessage(_GetSetRGBMessage(), 0, (LPARAM)clr);
	}

	// Get the selected color after DoModal returns, or in OnColorOK
	COLORREF GetColor() const
	{
		return m_cc.rgbResult;
	}

// Special override for the color dialog
	static UINT_PTR APIENTRY HookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if(uMsg != WM_INITDIALOG && uMsg != _GetColorOKMessage())
			return 0;

		LPCHOOSECOLOR lpCC = (LPCHOOSECOLOR)lParam;
		CCommonDialogImplBase* pT = NULL;

		if(uMsg == WM_INITDIALOG)
		{
			pT = (CCommonDialogImplBase*)_Module.ExtractCreateWndData();
			lpCC->lCustData = (LPARAM)pT;
			ATLASSERT(pT != NULL);
			ATLASSERT(pT->m_hWnd == NULL);
			ATLASSERT(::IsWindow(hWnd));
			// subclass dialog's window
			if(!pT->SubclassWindow(hWnd))
			{
				ATLTRACE2(atlTraceUI, 0, _T("Subclassing a Color common dialog failed\n"));
				return 0;
			}
		}
		else if(uMsg == _GetColorOKMessage())
		{
			pT = (CCommonDialogImplBase*)lpCC->lCustData;
			ATLASSERT(pT != NULL);
			ATLASSERT(::IsWindow(pT->m_hWnd));
		}

		// pass to the message map
		LRESULT lRes;
		if(pT->ProcessWindowMessage(pT->m_hWnd, uMsg, wParam, lParam, lRes, 0) == FALSE)
			return 0;
		return lRes;
	}

// Helpers
	static COLORREF* GetCustomColors()
	{
		static COLORREF rgbCustomColors[16] =
		{
			RGB(255, 255, 255), RGB(255, 255, 255), 
			RGB(255, 255, 255), RGB(255, 255, 255), 
			RGB(255, 255, 255), RGB(255, 255, 255), 
			RGB(255, 255, 255), RGB(255, 255, 255), 
			RGB(255, 255, 255), RGB(255, 255, 255), 
			RGB(255, 255, 255), RGB(255, 255, 255), 
			RGB(255, 255, 255), RGB(255, 255, 255), 
			RGB(255, 255, 255), RGB(255, 255, 255), 
		};

		return rgbCustomColors;
	}

	static UINT _GetSetRGBMessage()
	{
		static UINT uSetRGBMessage = 0;
		if(uSetRGBMessage == 0)
		{
			::EnterCriticalSection(&_Module.m_csStaticDataInit);
			if(uSetRGBMessage == 0)
				uSetRGBMessage = ::RegisterWindowMessage(SETRGBSTRING);
			::LeaveCriticalSection(&_Module.m_csStaticDataInit);
		}
		ATLASSERT(uSetRGBMessage != 0);
		return uSetRGBMessage;
	}

	static UINT _GetColorOKMessage()
	{
		static UINT uColorOKMessage = 0;
		if(uColorOKMessage == 0)
		{
			::EnterCriticalSection(&_Module.m_csStaticDataInit);
			if(uColorOKMessage == 0)
				uColorOKMessage = ::RegisterWindowMessage(COLOROKSTRING);
			::LeaveCriticalSection(&_Module.m_csStaticDataInit);
		}
		ATLASSERT(uColorOKMessage != 0);
		return uColorOKMessage;
	}

// Message map and handlers
	BEGIN_MSG_MAP(CColorDialogImpl)
		MESSAGE_HANDLER(_GetColorOKMessage(), _OnColorOK)
	END_MSG_MAP()

	LRESULT _OnColorOK(UINT, WPARAM, LPARAM, BOOL&)
	{
		T* pT = static_cast<T*>(this);
		return pT->OnColorOK();
	}

// Overrideable
	BOOL OnColorOK()        // validate color
	{
		return FALSE;
	}
};

class CColorDialog : public CColorDialogImpl<CColorDialog>
{
public:
	CColorDialog(COLORREF clrInit = 0, DWORD dwFlags = 0, HWND hWndParent = NULL)
		: CColorDialogImpl<CColorDialog>(clrInit, dwFlags, hWndParent)
	{ }

	// override base class map and references to handlers
	DECLARE_EMPTY_MSG_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CPrintDialogImpl - used for Print... and PrintSetup...

// global helper
static HDC _AtlCreateDC(HGLOBAL hDevNames, HGLOBAL hDevMode)
{
	if(hDevNames == NULL)
		return NULL;

	LPDEVNAMES lpDevNames = (LPDEVNAMES)::GlobalLock(hDevNames);
	LPDEVMODE  lpDevMode = (hDevMode != NULL) ? (LPDEVMODE)::GlobalLock(hDevMode) : NULL;

	if(lpDevNames == NULL)
		return NULL;

	HDC hDC = ::CreateDC((LPCTSTR)lpDevNames + lpDevNames->wDriverOffset,
					  (LPCTSTR)lpDevNames + lpDevNames->wDeviceOffset,
					  (LPCTSTR)lpDevNames + lpDevNames->wOutputOffset,
					  lpDevMode);

	::GlobalUnlock(hDevNames);
	if(hDevMode != NULL)
		::GlobalUnlock(hDevMode);
	return hDC;
}

template <class T>
class ATL_NO_VTABLE CPrintDialogImpl : public CCommonDialogImplBase
{
public:
	// print dialog parameter block (note this is a reference)
	PRINTDLG& m_pd;

// Constructors
	CPrintDialogImpl(BOOL bPrintSetupOnly = FALSE,	// TRUE for Print Setup, FALSE for Print Dialog
			DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS | PD_NOSELECTION,
			HWND hWndParent = NULL)
			: m_pd(m_pdActual)
	{
		memset(&m_pdActual, 0, sizeof(m_pdActual));

		m_pd.lStructSize = sizeof(m_pdActual);
		m_pd.hwndOwner = hWndParent;
		m_pd.Flags = (dwFlags | PD_ENABLEPRINTHOOK | PD_ENABLESETUPHOOK);
		m_pd.lpfnPrintHook = (LPPRINTHOOKPROC)T::HookProc;
		m_pd.lpfnSetupHook = (LPSETUPHOOKPROC)T::HookProc;

		if(bPrintSetupOnly)
			m_pd.Flags |= PD_PRINTSETUP;
		else
			m_pd.Flags |= PD_RETURNDC;

		m_pd.Flags &= ~PD_RETURNIC; // do not support information context
	}

// Operations
	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_pd.Flags & PD_ENABLEPRINTHOOK);
		ATLASSERT(m_pd.Flags & PD_ENABLESETUPHOOK);
		ATLASSERT(m_pd.lpfnPrintHook != NULL);	// can still be a user hook
		ATLASSERT(m_pd.lpfnSetupHook != NULL);	// can still be a user hook
		ATLASSERT((m_pd.Flags & PD_RETURNDEFAULT) == 0);	// use GetDefaults for this

		if(m_pd.hwndOwner == NULL)		// set only if not specified before
			m_pd.hwndOwner = hWndParent;

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)this);

		BOOL bRet = ::PrintDlg(&m_pd);

		m_hWnd = NULL;

		return bRet ? IDOK : IDCANCEL;
	}

	// GetDefaults will not display a dialog but will get device defaults
	BOOL GetDefaults()
	{
		m_pd.Flags |= PD_RETURNDEFAULT;
		ATLASSERT(m_pd.hDevMode == NULL);	// must be NULL
		ATLASSERT(m_pd.hDevNames == NULL);	// must be NULL

		return ::PrintDlg(&m_pd);
	}

	// Helpers for parsing information after successful return num. copies requested
	int GetCopies() const
	{
		if(m_pd.Flags & PD_USEDEVMODECOPIES)
		{
			LPDEVMODE lpDevMode = GetDevMode();
			return (lpDevMode != NULL) ? lpDevMode->dmCopies : -1;
		}

		return m_pd.nCopies;
	}
	BOOL PrintCollate() const       // TRUE if collate checked
	{
		return (m_pd.Flags & PD_COLLATE) ? TRUE : FALSE;
	}
	BOOL PrintSelection() const     // TRUE if printing selection
	{
		return (m_pd.Flags & PD_SELECTION) ? TRUE : FALSE;
	}
	BOOL PrintAll() const           // TRUE if printing all pages
	{
		return (!PrintRange() && !PrintSelection()) ? TRUE : FALSE;
	}
	BOOL PrintRange() const         // TRUE if printing page range
	{
		return (m_pd.Flags & PD_PAGENUMS) ? TRUE : FALSE;
	}
	int GetFromPage() const         // starting page if valid
	{
		return PrintRange() ? m_pd.nFromPage : -1;
	}
	int GetToPage() const           // ending page if valid
	{
		return PrintRange() ? m_pd.nToPage : -1;
	}
	LPDEVMODE GetDevMode() const    // return DEVMODE
	{
		if(m_pd.hDevMode == NULL)
			return NULL;

		return (LPDEVMODE)::GlobalLock(m_pd.hDevMode);
	}
	LPCTSTR GetDriverName() const   // return driver name
	{
		if(m_pd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(m_pd.hDevNames);
		if(lpDev == NULL)
			return NULL;

		return (LPCTSTR)lpDev + lpDev->wDriverOffset;
	}
	LPCTSTR GetDeviceName() const   // return device name
	{
		if(m_pd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(m_pd.hDevNames);
		if(lpDev == NULL)
			return NULL;

		return (LPCTSTR)lpDev + lpDev->wDeviceOffset;
	}
	LPCTSTR GetPortName() const     // return output port name
	{
		if(m_pd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(m_pd.hDevNames);
		if(lpDev == NULL)
			return NULL;

		return (LPCTSTR)lpDev + lpDev->wOutputOffset;
	}
	HDC GetPrinterDC() const        // return HDC (caller must delete)
	{
		ATLASSERT(m_pd.Flags & PD_RETURNDC);
		return m_pd.hDC;
	}

	// This helper creates a DC based on the DEVNAMES and DEVMODE structures.
	// This DC is returned, but also stored in m_pd.hDC as though it had been
	// returned by CommDlg.  It is assumed that any previously obtained DC
	// has been/will be deleted by the user.  This may be
	// used without ever invoking the print/print setup dialogs.
	HDC CreatePrinterDC()
	{
		m_pd.hDC = _AtlCreateDC(m_pd.hDevNames, m_pd.hDevMode);
		return m_pd.hDC;
	}

// Implementation
	PRINTDLG m_pdActual; // the Print/Print Setup need to share this

	// The following handle the case of print setup... from the print dialog
	CPrintDialogImpl(PRINTDLG& pdInit) : m_pd(pdInit)
	{ }

	BEGIN_MSG_MAP(CPrintDialogImpl)
#ifdef psh1
		COMMAND_ID_HANDLER(psh1, OnPrintSetup) // print setup button when print is displayed
#else //!psh1
		COMMAND_ID_HANDLER(0x0400, OnPrintSetup) // value from dlgs.h
#endif //!psh1
	END_MSG_MAP()

	LRESULT OnPrintSetup(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
	{
		CPrintDialogImpl< T >* pDlgSetup = NULL;
		ATLTRY(pDlgSetup = new CPrintDialogImpl< T >(m_pd));
		ATLASSERT(pDlgSetup != NULL);

		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)pDlgSetup);
		LRESULT lRet = DefWindowProc(WM_COMMAND, MAKEWPARAM(wID, wNotifyCode), (LPARAM)hWndCtl);

		delete pDlgSetup;
		return lRet;
	}
};

class CPrintDialog : public CPrintDialogImpl<CPrintDialog>
{
public:
	CPrintDialog(BOOL bPrintSetupOnly = FALSE,
		DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS | PD_NOSELECTION,
		HWND hWndParent = NULL)
		: CPrintDialogImpl<CPrintDialog>(bPrintSetupOnly, dwFlags, hWndParent)
	{ }

	CPrintDialog(PRINTDLG& pdInit) : CPrintDialogImpl<CPrintDialog>(pdInit)
	{ }
};


/////////////////////////////////////////////////////////////////////////////
// CPrintDialogExImpl - new print dialog for Windows 2000

#if (WINVER >= 0x0500)

}; //namespace WTL

#include <atlcom.h>

extern "C" const __declspec(selectany) IID IID_IPrintDialogCallback = {0x5852a2c3, 0x6530, 0x11d1, {0xb6, 0xa3, 0x0, 0x0, 0xf8, 0x75, 0x7b, 0xf9}};
extern "C" const __declspec(selectany) IID IID_IPrintDialogServices = {0x509aaeda, 0x5639, 0x11d1, {0xb6, 0xa1, 0x0, 0x0, 0xf8, 0x75, 0x7b, 0xf9}};

namespace WTL
{

template <class T>
class ATL_NO_VTABLE CPrintDialogExImpl : 
				public CWindow,
				public CMessageMap,
				public IPrintDialogCallback,
				public IObjectWithSiteImpl< T >
{
public:
	PRINTDLGEX m_pdex;

// Constructor
	CPrintDialogExImpl(DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS | PD_NOSELECTION | PD_NOCURRENTPAGE,
				HWND hWndParent = NULL)
	{
		memset(&m_pdex, 0, sizeof(m_pdex));

		m_pdex.lStructSize = sizeof(PRINTDLGEX);
		m_pdex.hwndOwner = hWndParent;
		m_pdex.Flags = dwFlags;
		m_pdex.nStartPage = START_PAGE_GENERAL;
		// callback object will be set in DoModal

		m_pdex.Flags &= ~PD_RETURNIC; // do not support information context
	}

// Operations
	HRESULT DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_hWnd == NULL);
		ATLASSERT((m_pdex.Flags & PD_RETURNDEFAULT) == 0);	// use GetDefaults for this

		if(m_pdex.hwndOwner == NULL)		// set only if not specified before
			m_pdex.hwndOwner = hWndParent;

		T* pT = static_cast<T*>(this);
		m_pdex.lpCallback = (IUnknown*)(IPrintDialogCallback*)pT;

		HRESULT hResult = ::PrintDlgEx(&m_pdex);

		m_hWnd = NULL;

		return hResult;
	}

	BOOL EndDialog(INT_PTR /*nRetCode*/ = 0)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		SendMessage(WM_COMMAND, MAKEWPARAM(IDABORT, 0));
		return TRUE;
	}

	// GetDefaults will not display a dialog but will get device defaults
	HRESULT GetDefaults()
	{
		m_pdex.Flags |= PD_RETURNDEFAULT;
		ATLASSERT(m_pdex.hDevMode == NULL);	// must be NULL
		ATLASSERT(m_pdex.hDevNames == NULL);	// must be NULL

		return ::PrintDlgEx(&m_pdex);
	}

	// Helpers for parsing information after successful return num. copies requested
	int GetCopies() const
	{
		if(m_pdex.Flags & PD_USEDEVMODECOPIES)
		{
			LPDEVMODE lpDevMode = GetDevMode();
			return (lpDevMode != NULL) ? lpDevMode->dmCopies : -1;
		}

		return m_pdex.nCopies;
	}
	BOOL PrintCollate() const       // TRUE if collate checked
	{
		return (m_pdex.Flags & PD_COLLATE) ? TRUE : FALSE;
	}
	BOOL PrintSelection() const     // TRUE if printing selection
	{
		return (m_pdex.Flags & PD_SELECTION) ? TRUE : FALSE;
	}
	BOOL PrintAll() const           // TRUE if printing all pages
	{
		return (!PrintRange() && !PrintSelection()) ? TRUE : FALSE;
	}
	BOOL PrintRange() const         // TRUE if printing page range
	{
		return (m_pdex.Flags & PD_PAGENUMS) ? TRUE : FALSE;
	}
	LPDEVMODE GetDevMode() const    // return DEVMODE
	{
		if(m_pdex.hDevMode == NULL)
			return NULL;

		return (LPDEVMODE)::GlobalLock(m_pdex.hDevMode);
	}
	LPCTSTR GetDriverName() const   // return driver name
	{
		if(m_pdex.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(m_pdex.hDevNames);
		if(lpDev == NULL)
			return NULL;

		return (LPCTSTR)lpDev + lpDev->wDriverOffset;
	}
	LPCTSTR GetDeviceName() const   // return device name
	{
		if(m_pdex.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(m_pdex.hDevNames);
		if(lpDev == NULL)
			return NULL;

		return (LPCTSTR)lpDev + lpDev->wDeviceOffset;
	}
	LPCTSTR GetPortName() const     // return output port name
	{
		if(m_pdex.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(m_pdex.hDevNames);
		if(lpDev == NULL)
			return NULL;

		return (LPCTSTR)lpDev + lpDev->wOutputOffset;
	}
	HDC GetPrinterDC() const        // return HDC (caller must delete)
	{
		ATLASSERT(m_pdex.Flags & PD_RETURNDC);
		return m_pdex.hDC;
	}

	// This helper creates a DC based on the DEVNAMES and DEVMODE structures.
	// This DC is returned, but also stored in m_pdex.hDC as though it had been
	// returned by CommDlg.  It is assumed that any previously obtained DC
	// has been/will be deleted by the user.  This may be
	// used without ever invoking the print/print setup dialogs.
	HDC CreatePrinterDC()
	{
		m_pdex.hDC = _AtlCreateDC(m_pdex.hDevNames, m_pdex.hDevMode);
		return m_pdex.hDC;
	}

// Implementation - interfaces

// IUnknown
	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
	{
		if(ppvObject == NULL)
			return E_POINTER;

		T* pT = static_cast<T*>(this);
		if(IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, IID_IPrintDialogCallback))
		{
			*ppvObject = (IPrintDialogCallback*)pT;
			// AddRef() not needed
			return S_OK;
		}
		else if(IsEqualGUID(riid, IID_IObjectWithSite))
		{
			*ppvObject = (IObjectWithSite*)pT;
			// AddRef() not needed
			return S_OK;
		}

		return E_NOINTERFACE;
	}
	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return 1;
	}
	virtual ULONG STDMETHODCALLTYPE Release()
	{
		return 1;
	}

// IPrintDialogCallback
	STDMETHOD(InitDone)()
	{
		return S_FALSE;
	}
	STDMETHOD(SelectionChange)()
	{
		return S_FALSE;
	}
	STDMETHOD(HandleMessage)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
	{
		// set up m_hWnd the first time
		if(m_hWnd == NULL)
			Attach(hWnd);

		// call message map
		HRESULT hRet = ProcessWindowMessage(hWnd, uMsg, wParam, lParam, *plResult, 0) ? S_OK : S_FALSE;
		if(hRet == S_OK && uMsg == WM_NOTIFY)	// return in DWLP_MSGRESULT
			::SetWindowLongPtr(GetParent(), DWLP_MSGRESULT, (LONG_PTR)*plResult);

		if(uMsg == WM_INITDIALOG && hRet == S_OK && (BOOL)*plResult != FALSE)
			hRet = S_FALSE;

		return hRet;
	}
};

class CPrintDialogEx : public CPrintDialogExImpl<CPrintDialogEx>
{
public:
	CPrintDialogEx(
		DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS | PD_NOSELECTION | PD_NOCURRENTPAGE,
		HWND hWndParent = NULL)
		: CPrintDialogExImpl<CPrintDialogEx>(dwFlags, hWndParent)
	{ }

	DECLARE_EMPTY_MSG_MAP()
};

#endif //(WINVER >= 0x0500)

/////////////////////////////////////////////////////////////////////////////
// CPageSetupDialogImpl - Page Setup dialog

template <class T>
class ATL_NO_VTABLE CPageSetupDialogImpl : public CCommonDialogImplBase
{
public:
	PAGESETUPDLG m_psd;
	CWndProcThunk m_thunkPaint;


// Constructors
	CPageSetupDialogImpl(DWORD dwFlags = PSD_MARGINS | PSD_INWININIINTLMEASURE, HWND hWndParent = NULL)
	{
		memset(&m_psd, 0, sizeof(m_psd));

		m_psd.lStructSize = sizeof(m_psd);
		m_psd.hwndOwner = hWndParent;
		m_psd.Flags = (dwFlags | PSD_ENABLEPAGESETUPHOOK | PSD_ENABLEPAGEPAINTHOOK);
		m_psd.lpfnPageSetupHook = (LPPAGESETUPHOOK)T::HookProc;
		m_thunkPaint.Init((WNDPROC)T::PaintHookProc, this);
#if (_ATL_VER >= 0x0700)
		m_psd.lpfnPagePaintHook = (LPPAGEPAINTHOOK)m_thunkPaint.GetWNDPROC();
#else
		m_psd.lpfnPagePaintHook = (LPPAGEPAINTHOOK)&(m_thunkPaint.thunk);
#endif
	}

	DECLARE_EMPTY_MSG_MAP()

// Attributes
	LPDEVMODE GetDevMode() const    // return DEVMODE
	{
		if(m_psd.hDevMode == NULL)
			return NULL;

		return (LPDEVMODE)::GlobalLock(m_psd.hDevMode);
	}
	LPCTSTR GetDriverName() const   // return driver name
	{
		if(m_psd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(m_psd.hDevNames);
		return (LPCTSTR)lpDev + lpDev->wDriverOffset;
	}
	LPCTSTR GetDeviceName() const   // return device name
	{
		if(m_psd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(m_psd.hDevNames);
		return (LPCTSTR)lpDev + lpDev->wDeviceOffset;
	}
	LPCTSTR GetPortName() const     // return output port name
	{
		if(m_psd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)::GlobalLock(m_psd.hDevNames);
		return (LPCTSTR)lpDev + lpDev->wOutputOffset;
	}
	HDC CreatePrinterDC()
	{
		return _AtlCreateDC(m_psd.hDevNames, m_psd.hDevMode);
	}
	SIZE GetPaperSize() const
	{
		SIZE size;
		size.cx = m_psd.ptPaperSize.x;
		size.cy = m_psd.ptPaperSize.y;
		return size;
	}
	void GetMargins(LPRECT lpRectMargins, LPRECT lpRectMinMargins) const
	{
		if(lpRectMargins != NULL)
			memcpy(lpRectMargins, &m_psd.rtMargin, sizeof(RECT));
		if(lpRectMinMargins != NULL)
			memcpy(lpRectMinMargins, &m_psd.rtMinMargin, sizeof(RECT));
	}

// Operations
	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_psd.Flags & PSD_ENABLEPAGESETUPHOOK);
		ATLASSERT(m_psd.Flags & PSD_ENABLEPAGEPAINTHOOK);
		ATLASSERT(m_psd.lpfnPageSetupHook != NULL);	// can still be a user hook
		ATLASSERT(m_psd.lpfnPagePaintHook != NULL);	// can still be a user hook

		if(m_psd.hwndOwner == NULL)		// set only if not specified before
			m_psd.hwndOwner = hWndParent;

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)this);

		BOOL bRet = ::PageSetupDlg(&m_psd);

		m_hWnd = NULL;

		return bRet ? IDOK : IDCANCEL;
	}

// Implementation
	static UINT CALLBACK PaintHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CPageSetupDialogImpl< T >* pDlg = (CPageSetupDialogImpl< T >*)hWnd;
		ATLASSERT(pDlg->m_hWnd == ::GetParent(hWnd));
		UINT uRet = 0;
		switch(uMsg)
		{
		case WM_PSD_PAGESETUPDLG:
			uRet = pDlg->PreDrawPage(LOWORD(wParam), HIWORD(wParam), (LPPAGESETUPDLG)lParam);
			break;
		case WM_PSD_FULLPAGERECT:
		case WM_PSD_MINMARGINRECT:
		case WM_PSD_MARGINRECT:
		case WM_PSD_GREEKTEXTRECT:
		case WM_PSD_ENVSTAMPRECT:
		case WM_PSD_YAFULLPAGERECT:
			uRet = pDlg->OnDrawPage(uMsg, (HDC)wParam, (LPRECT)lParam);
			break;
		default:
			ATLTRACE2(atlTraceUI, 0, _T("CPageSetupDialogImpl::PaintHookProc - unknown message received\n"));
			break;
		}
		return uRet;
	}

// Overridables
	UINT PreDrawPage(WORD /*wPaper*/, WORD /*wFlags*/, LPPAGESETUPDLG /*pPSD*/)
	{
		// return 1 to prevent any more drawing
		return 0;
	}
	UINT OnDrawPage(UINT /*uMsg*/, HDC /*hDC*/, LPRECT /*lpRect*/)
	{
		return 0; // do the default
	}
};

class CPageSetupDialog : public CPageSetupDialogImpl<CPageSetupDialog>
{
public:
	CPageSetupDialog(DWORD dwFlags = PSD_MARGINS | PSD_INWININIINTLMEASURE, HWND hWndParent = NULL)
		: CPageSetupDialogImpl<CPageSetupDialog>(dwFlags, hWndParent)
	{ }

	// override PaintHookProc and references to handlers
	static UINT CALLBACK PaintHookProc(HWND, UINT, WPARAM, LPARAM)
	{
		return 0;
	}
};


/////////////////////////////////////////////////////////////////////////////
// CFindReplaceDialogImpl - Find/FindReplace modeless dialogs

template <class T>
class ATL_NO_VTABLE CFindReplaceDialogImpl : public CCommonDialogImplBase
{
public:
	FINDREPLACE m_fr;
	TCHAR m_szFindWhat[128];
	TCHAR m_szReplaceWith[128];

// Constructors
	CFindReplaceDialogImpl()
	{
		memset(&m_fr, 0, sizeof(m_fr));
		m_szFindWhat[0] = '\0';
		m_szReplaceWith[0] = '\0';

		m_fr.lStructSize = sizeof(m_fr);
		m_fr.Flags = FR_ENABLEHOOK;
		m_fr.lpfnHook = (LPFRHOOKPROC)T::HookProc;
		m_fr.lpstrFindWhat = (LPTSTR)m_szFindWhat;
	}

	// Note: You must allocate the object on the heap.
	//       If you do not, you must override OnFinalMessage()
	virtual void OnFinalMessage(HWND /*hWnd*/)
	{
		delete this;
	}

	HWND Create(BOOL bFindDialogOnly, // TRUE for Find, FALSE for FindReplace
			LPCTSTR lpszFindWhat,
			LPCTSTR lpszReplaceWith = NULL,
			DWORD dwFlags = FR_DOWN,
			HWND hWndParent = NULL)
	{
		ATLASSERT(m_fr.Flags & FR_ENABLEHOOK);
		ATLASSERT(m_fr.lpfnHook != NULL);

		m_fr.wFindWhatLen = sizeof(m_szFindWhat)/sizeof(TCHAR);
		m_fr.lpstrReplaceWith = (LPTSTR)m_szReplaceWith;
		m_fr.wReplaceWithLen = sizeof(m_szReplaceWith)/sizeof(TCHAR);
		m_fr.Flags |= dwFlags;

		if(hWndParent == NULL)
			m_fr.hwndOwner = ::GetActiveWindow();
		else
			m_fr.hwndOwner = hWndParent;
		ATLASSERT(m_fr.hwndOwner != NULL); // must have an owner for modeless dialog

		if(lpszFindWhat != NULL)
			lstrcpyn(m_szFindWhat, lpszFindWhat, sizeof(m_szFindWhat)/sizeof(TCHAR));

		if(lpszReplaceWith != NULL)
			lstrcpyn(m_szReplaceWith, lpszReplaceWith, sizeof(m_szReplaceWith)/sizeof(TCHAR));

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)this);
		HWND hWnd;

		if(bFindDialogOnly)
			hWnd = ::FindText(&m_fr);
		else
			hWnd = ::ReplaceText(&m_fr);

		ATLASSERT(m_hWnd == hWnd);
		return hWnd;
	}

	static const UINT GetFindReplaceMsg()
	{
		static const UINT nMsgFindReplace = ::RegisterWindowMessage(FINDMSGSTRING);
		return nMsgFindReplace;
	}
	// call while handling FINDMSGSTRING registered message
	// to retreive the object
	static T* PASCAL GetNotifier(LPARAM lParam)
	{
		ATLASSERT(lParam != NULL);
		T* pDlg = (T*)(lParam - offsetof(T, m_fr));
		return pDlg;
	}

// Operations
	// Helpers for parsing information after successful return
	LPCTSTR GetFindString() const    // get find string
	{
		return (LPCTSTR)m_fr.lpstrFindWhat;
	}
	LPCTSTR GetReplaceString() const // get replacement string
	{
		return (LPCTSTR)m_fr.lpstrReplaceWith;
	}
	BOOL SearchDown() const          // TRUE if search down, FALSE is up
	{
		return (m_fr.Flags & FR_DOWN) ? TRUE : FALSE;
	}
	BOOL FindNext() const            // TRUE if command is find next
	{
		return (m_fr.Flags & FR_FINDNEXT) ? TRUE : FALSE;
	}
	BOOL MatchCase() const           // TRUE if matching case
	{
		return (m_fr.Flags & FR_MATCHCASE) ? TRUE : FALSE;
	}
	BOOL MatchWholeWord() const      // TRUE if matching whole words only
	{
		return (m_fr.Flags & FR_WHOLEWORD) ? TRUE : FALSE;
	}
	BOOL ReplaceCurrent() const      // TRUE if replacing current string
	{
		return (m_fr. Flags & FR_REPLACE) ? TRUE : FALSE;
	}
	BOOL ReplaceAll() const          // TRUE if replacing all occurrences
	{
		return (m_fr.Flags & FR_REPLACEALL) ? TRUE : FALSE;
	}
	BOOL IsTerminating() const       // TRUE if terminating dialog
	{
		return (m_fr.Flags & FR_DIALOGTERM) ? TRUE : FALSE ;
	}
};

class CFindReplaceDialog : public CFindReplaceDialogImpl<CFindReplaceDialog>
{
public:
	DECLARE_EMPTY_MSG_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CPropertySheetWindow - client side for a property sheet

class CPropertySheetWindow : public CWindow
{
public:
// Constructors
	CPropertySheetWindow(HWND hWnd = NULL) : CWindow(hWnd) { }

	CPropertySheetWindow& operator=(HWND hWnd)
	{
		m_hWnd = hWnd;
		return *this;
	}

// Attributes
	int GetPageCount() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		HWND hWndTabCtrl = GetTabControl();
		ATLASSERT(hWndTabCtrl != NULL);
		return (int)::SendMessage(hWndTabCtrl, TCM_GETITEMCOUNT, 0, 0L);
	}
	HWND GetActivePage() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HWND)::SendMessage(m_hWnd, PSM_GETCURRENTPAGEHWND, 0, 0L);
	}
	int GetActiveIndex() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		HWND hWndTabCtrl = GetTabControl();
		ATLASSERT(hWndTabCtrl != NULL);
		return (int)::SendMessage(hWndTabCtrl, TCM_GETCURSEL, 0, 0L);
	}
	BOOL SetActivePage(int nPageIndex)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_SETCURSEL, nPageIndex, 0L);
	}
	BOOL SetActivePage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(hPage != NULL);
		return (BOOL)::SendMessage(m_hWnd, PSM_SETCURSEL, 0, (LPARAM)hPage);
	}
	BOOL SetActivePageByID(int nPageID)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_SETCURSELID, 0, nPageID);
	}
	void SetTitle(LPCTSTR lpszText, UINT nStyle = 0)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT((nStyle & ~PSH_PROPTITLE) == 0); // only PSH_PROPTITLE is valid
		ATLASSERT(lpszText != NULL);
		::SendMessage(m_hWnd, PSM_SETTITLE, nStyle, (LPARAM)lpszText);
	}
	HWND GetTabControl() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HWND)::SendMessage(m_hWnd, PSM_GETTABCONTROL, 0, 0L);
	}
	void SetFinishText(LPCTSTR lpszText)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_SETFINISHTEXT, 0, (LPARAM)lpszText);
	}
	void SetWizardButtons(DWORD dwFlags)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::PostMessage(m_hWnd, PSM_SETWIZBUTTONS, 0, dwFlags);
	}

// Operations
	void AddPage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(hPage != NULL);
		::SendMessage(m_hWnd, PSM_ADDPAGE, 0, (LPARAM)hPage);
	}
	BOOL AddPage(LPCPROPSHEETPAGE pPage)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(pPage != NULL);
		HPROPSHEETPAGE hPage = ::CreatePropertySheetPage(pPage);
		if(hPage == NULL)
			return FALSE;
		::SendMessage(m_hWnd, PSM_ADDPAGE, 0, (LPARAM)hPage);
		return TRUE;
	}
	void RemovePage(int nPageIndex)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_REMOVEPAGE, nPageIndex, 0L);
	}
	void RemovePage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(hPage != NULL);
		::SendMessage(m_hWnd, PSM_REMOVEPAGE, 0, (LPARAM)hPage);
	}
	BOOL PressButton(int nButton)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_PRESSBUTTON, nButton, 0L);
	}
	BOOL Apply()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_APPLY, 0, 0L);
	}
	void CancelToClose()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_CANCELTOCLOSE, 0, 0L);
	}
	void SetModified(HWND hWndPage, BOOL bChanged = TRUE)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(::IsWindow(hWndPage));
		UINT uMsg = bChanged ? PSM_CHANGED : PSM_UNCHANGED;
		::SendMessage(m_hWnd, uMsg, (WPARAM)hWndPage, 0L);
	}
	LRESULT QuerySiblings(WPARAM wParam, LPARAM lParam)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SendMessage(m_hWnd, PSM_QUERYSIBLINGS, wParam, lParam);
	}
	void RebootSystem()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_REBOOTSYSTEM, 0, 0L);
	}
	void RestartWindows()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_RESTARTWINDOWS, 0, 0L);
	}
	BOOL IsDialogMessage(LPMSG lpMsg)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_ISDIALOGMESSAGE, 0, (LPARAM)lpMsg);
	}

#if (_WIN32_IE >= 0x0500)
	int HwndToIndex(HWND hWnd) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (int)::SendMessage(m_hWnd, PSM_HWNDTOINDEX, (WPARAM)hWnd, 0L);
	}
	HWND IndexToHwnd(int nIndex) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HWND)::SendMessage(m_hWnd, PSM_INDEXTOHWND, nIndex, 0L);
	}
	int PageToIndex(HPROPSHEETPAGE hPage) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (int)::SendMessage(m_hWnd, PSM_PAGETOINDEX, 0, (LPARAM)hPage);
	}
	HPROPSHEETPAGE IndexToPage(int nIndex) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HPROPSHEETPAGE)::SendMessage(m_hWnd, PSM_INDEXTOPAGE, nIndex, 0L);
	}
	int IdToIndex(int nID) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (int)::SendMessage(m_hWnd, PSM_IDTOINDEX, 0, nID);
	}
	int IndexToId(int nIndex) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (int)::SendMessage(m_hWnd, PSM_INDEXTOID, nIndex, 0L);
	}
	int GetResult() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (int)::SendMessage(m_hWnd, PSM_GETRESULT, 0, 0L);
	}
	BOOL RecalcPageSizes()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_RECALCPAGESIZES, 0, 0L);
	}
#endif //(_WIN32_IE >= 0x0500)

// Implementation - override to prevent usage
	HWND Create(LPCTSTR, HWND, _U_RECT = NULL, LPCTSTR = NULL, DWORD = 0, DWORD = 0, _U_MENUorID = 0U, LPVOID = NULL)
	{
		ATLASSERT(FALSE);
		return NULL;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CPropertySheetImpl - implements a property sheet

#if (_MSC_VER >= 1200)
typedef HPROPSHEETPAGE	_HPROPSHEETPAGE_TYPE;
#else
// we use void* here instead of HPROPSHEETPAGE becuase HPROPSHEETPAGE
// is a _PSP*, but _PSP is not defined properly
typedef void*	_HPROPSHEETPAGE_TYPE;
#endif

template <class T, class TBase = CPropertySheetWindow>
class ATL_NO_VTABLE CPropertySheetImpl : public CWindowImplBaseT< TBase >
{
public:
	PROPSHEETHEADER m_psh;
	CSimpleArray<_HPROPSHEETPAGE_TYPE> m_arrPages;

// Construction/Destruction
	CPropertySheetImpl(_U_STRINGorID title = (LPCTSTR)NULL, UINT uStartPage = 0, HWND hWndParent = NULL)
	{
		memset(&m_psh, 0, sizeof(PROPSHEETHEADER));
		m_psh.dwSize = sizeof(PROPSHEETHEADER);
		m_psh.dwFlags = PSH_USECALLBACK;
		m_psh.hInstance = _Module.GetResourceInstance();
		m_psh.phpage = NULL;	// will be set later
		m_psh.nPages = 0;	// will be set later
		m_psh.pszCaption = title.m_lpstr;
		m_psh.nStartPage = uStartPage;
		m_psh.hwndParent = hWndParent;	// if NULL, will be set in DoModal/Create
		m_psh.pfnCallback = T::PropSheetCallback;
	}

	~CPropertySheetImpl()
	{
		if(m_arrPages.GetSize() > 0)	// sheet never created, destroy all pages
		{
			for(int i = 0; i < m_arrPages.GetSize(); i++)
				::DestroyPropertySheetPage((HPROPSHEETPAGE)m_arrPages[i]);
		}
	}

	static int CALLBACK PropSheetCallback(HWND hWnd, UINT uMsg, LPARAM)
	{
		if(uMsg == PSCB_INITIALIZED)
		{
			ATLASSERT(hWnd != NULL);
			T* pT = (T*)_Module.ExtractCreateWndData();
			// subclass the sheet window
			pT->SubclassWindow(hWnd);
			// remove page handles array
			pT->_CleanUpPages();
		}

		return 0;
	}

	HWND Create(HWND hWndParent = NULL)
	{
		ATLASSERT(m_hWnd == NULL);

		m_psh.dwFlags |= PSH_MODELESS;
		if(m_psh.hwndParent == NULL)
			m_psh.hwndParent = hWndParent;
		m_psh.phpage = (HPROPSHEETPAGE*)m_arrPages.GetData();
		m_psh.nPages = m_arrPages.GetSize();

		T* pT = static_cast<T*>(this);
		_Module.AddCreateWndData(&m_thunk.cd, pT);

		HWND hWnd = (HWND)::PropertySheet(&m_psh);
		_CleanUpPages();	// ensure clean-up, required if call failed

		ATLASSERT(m_hWnd == hWnd);

		return hWnd;
	}

	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_hWnd == NULL);

		m_psh.dwFlags &= ~PSH_MODELESS;
		if(m_psh.hwndParent == NULL)
			m_psh.hwndParent = hWndParent;
		m_psh.phpage = (HPROPSHEETPAGE*)m_arrPages.GetData();
		m_psh.nPages = m_arrPages.GetSize();

		T* pT = static_cast<T*>(this);
		_Module.AddCreateWndData(&m_thunk.cd, pT);

		INT_PTR nRet = ::PropertySheet(&m_psh);
		_CleanUpPages();	// ensure clean-up, required if call failed

		return nRet;
	}

	// implementation helper - clean up pages array
	void _CleanUpPages()
	{
		m_psh.nPages = 0;
		m_psh.phpage = NULL;
		m_arrPages.RemoveAll();
	}

// Attributes (extended overrides of client class methods)
// These now can be called before the sheet is created
// Note: Calling these after the sheet is created gives unpredictable results
	int GetPageCount() const
	{
		if(m_hWnd == NULL)	// not created yet
			return m_arrPages.GetSize();
		return TBase::GetPageCount();
	}
	int GetActiveIndex() const
	{
		if(m_hWnd == NULL)	// not created yet
			return m_psh.nStartPage;
		return TBase::GetActiveIndex();
	}
	HPROPSHEETPAGE GetPage(int nPageIndex) const
	{
		ATLASSERT(m_hWnd == NULL);	// can't do this after it's created
		return (HPROPSHEETPAGE)m_arrPages[nPageIndex];
	}
	int GetPageIndex(HPROPSHEETPAGE hPage) const
	{
		ATLASSERT(m_hWnd == NULL);	// can't do this after it's created
		return m_arrPages.Find((_HPROPSHEETPAGE_TYPE&)hPage);
	}
	BOOL SetActivePage(int nPageIndex)
	{
		if(m_hWnd == NULL)	// not created yet
		{
			ATLASSERT(nPageIndex >= 0 && nPageIndex < m_arrPages.GetSize());
			m_psh.nStartPage = nPageIndex;
			return TRUE;
		}
		return TBase::SetActivePage(nPageIndex);
	}
	BOOL SetActivePage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(hPage != NULL);
		if (m_hWnd == NULL)	// not created yet
		{
			int nPageIndex = GetPageIndex(hPage);
			if(nPageIndex == -1)
				return FALSE;

			return SetActivePage(nPageIndex);
		}
		return TBase::SetActivePage(hPage);

	}
	void SetTitle(LPCTSTR lpszText, UINT nStyle = 0)
	{
		ATLASSERT((nStyle & ~PSH_PROPTITLE) == 0); // only PSH_PROPTITLE is valid
		ATLASSERT(lpszText != NULL);

		if(m_hWnd == NULL)
		{
			// set internal state
			m_psh.pszCaption = lpszText;	// must exist until sheet is created
			m_psh.dwFlags &= ~PSH_PROPTITLE;
			m_psh.dwFlags |= nStyle;
		}
		else
		{
			// set external state
			TBase::SetTitle(lpszText, nStyle);
		}
	}
	void SetWizardMode()
	{
		m_psh.dwFlags |= PSH_WIZARD;
	}
	void EnableHelp()
	{
		m_psh.dwFlags |= PSH_HASHELP;
	}

// Operations
	BOOL AddPage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(hPage != NULL);
		BOOL bRet = TRUE;
		if(m_hWnd != NULL)
			TBase::AddPage(hPage);
		else	// sheet not created yet, use internal data
			bRet = m_arrPages.Add((_HPROPSHEETPAGE_TYPE&)hPage);
		return bRet;
	}
	BOOL AddPage(LPCPROPSHEETPAGE pPage)
	{
		ATLASSERT(pPage != NULL);
		HPROPSHEETPAGE hPage = ::CreatePropertySheetPage(pPage);
		if(hPage == NULL)
			return FALSE;
		BOOL bRet = AddPage(hPage);
		if(!bRet)
			::DestroyPropertySheetPage(hPage);
		return bRet;
	}
	BOOL RemovePage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(hPage != NULL);
		if (m_hWnd == NULL)		// not created yet
		{
			int nPage = GetPageIndex(hPage);
			if(nPage == -1)
				return FALSE;
			return RemovePage(nPage);
		}
		TBase::RemovePage(hPage);
		return TRUE;

	}
	BOOL RemovePage(int nPageIndex)
	{
		BOOL bRet = TRUE;
		if(m_hWnd != NULL)
			TBase::RemovePage(nPageIndex);
		else	// sheet not created yet, use internal data
			bRet = m_arrPages.RemoveAt(nPageIndex);
		return bRet;
	}

#if (_WIN32_IE >= 0x0400)
	void SetHeader(LPCTSTR szbmHeader)
	{
		ATLASSERT(m_hWnd == NULL);	// can't do this after it's created

		m_psh.dwFlags &= ~PSH_WIZARD;
		m_psh.dwFlags |= (PSH_HEADER | PSH_WIZARD97);
		m_psh.pszbmHeader = szbmHeader;
	}

	void SetHeader(HBITMAP hbmHeader)
	{
		ATLASSERT(m_hWnd == NULL);	// can't do this after it's created

		m_psh.dwFlags &= ~PSH_WIZARD;
		m_psh.dwFlags |= (PSH_HEADER | PSH_USEHBMHEADER | PSH_WIZARD97);
		m_psh.hbmHeader = hbmHeader;
	}

	void SetWatermark(LPCTSTR szbmWatermark, HPALETTE hplWatermark = NULL)
	{
		ATLASSERT(m_hWnd == NULL);	// can't do this after it's created

		m_psh.dwFlags &= ~PSH_WIZARD;
		m_psh.dwFlags |= PSH_WATERMARK | PSH_WIZARD97;
		m_psh.pszbmWatermark = szbmWatermark;

		if (hplWatermark != NULL)
		{
			m_psh.dwFlags |= PSH_USEHPLWATERMARK;
			m_psh.hplWatermark = hplWatermark;
		}
	}

	void SetWatermark(HBITMAP hbmWatermark, HPALETTE hplWatermark = NULL)
	{
		ATLASSERT(m_hWnd == NULL);	// can't do this after it's created

		m_psh.dwFlags &= ~PSH_WIZARD;
		m_psh.dwFlags |= (PSH_WATERMARK | PSH_USEHBMWATERMARK | PSH_WIZARD97);
		m_psh.hbmWatermark = hbmWatermark;

		if (hplWatermark != NULL)
		{
			m_psh.dwFlags |= PSH_USEHPLWATERMARK;
			m_psh.hplWatermark = hplWatermark;
		}
	}

	void StretchWatermark(bool bStretchWatermark)
	{
		ATLASSERT(m_hWnd == NULL);	// can't do this after it's created
		if (bStretchWatermark)
			m_psh.dwFlags |= PSH_STRETCHWATERMARK;
		else
			m_psh.dwFlags &= ~PSH_STRETCHWATERMARK;
	}
#endif

// Message map and handlers
	BEGIN_MSG_MAP(CPropertySheetImpl)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
	END_MSG_MAP()

	LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);
		if(HIWORD(wParam) == BN_CLICKED && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) &&
		   ((m_psh.dwFlags & PSH_MODELESS) != 0) && (GetActivePage() == NULL))
			DestroyWindow();
		return lRet;
	}

	LRESULT OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(((m_psh.dwFlags & PSH_MODELESS) == PSH_MODELESS) && ((wParam & 0xFFF0) == SC_CLOSE))
			SendMessage(WM_CLOSE);
		else
			bHandled = FALSE;
		return 0;
	}
};

// for non-customized sheets
class CPropertySheet : public CPropertySheetImpl<CPropertySheet>
{
public:
	CPropertySheet(_U_STRINGorID title = (LPCTSTR)NULL, UINT uStartPage = 0, HWND hWndParent = NULL)
		: CPropertySheetImpl<CPropertySheet>(title, uStartPage, hWndParent)
	{ }

	BEGIN_MSG_MAP(CPropertySheet)
		MESSAGE_HANDLER(WM_COMMAND, CPropertySheetImpl<CPropertySheet>::OnCommand)
	END_MSG_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CPropertyPageWindow - client side for a property page

class CPropertyPageWindow : public CWindow
{
public:
// Constructors
	CPropertyPageWindow(HWND hWnd = NULL) : CWindow(hWnd) { }

	CPropertyPageWindow& operator=(HWND hWnd)
	{
		m_hWnd = hWnd;
		return *this;
	}

// Attributes
	CPropertySheetWindow GetPropertySheet() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CPropertySheetWindow(GetParent());
	}

// Operations
	BOOL Apply()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		return GetPropertySheet().Apply();
	}
	void CancelToClose()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		GetPropertySheet().CancelToClose();
	}
	void SetModified(BOOL bChanged = TRUE)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		GetPropertySheet().SetModified(m_hWnd, bChanged);
	}
	LRESULT QuerySiblings(WPARAM wParam, LPARAM lParam)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		return GetPropertySheet().QuerySiblings(wParam, lParam);
	}
	void RebootSystem()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		GetPropertySheet().RebootSystem();
	}
	void RestartWindows()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		GetPropertySheet().RestartWindows();
	}
	void SetWizardButtons(DWORD dwFlags)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		GetPropertySheet().SetWizardButtons(dwFlags);
	}

// Implementation - overrides to prevent usage
	HWND Create(LPCTSTR, HWND, _U_RECT = NULL, LPCTSTR = NULL, DWORD = 0, DWORD = 0, _U_MENUorID = 0U, LPVOID = NULL)
	{
		ATLASSERT(FALSE);
		return NULL;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CPropertyPageImpl - implements a property page

template <class T, class TBase = CPropertyPageWindow>
class ATL_NO_VTABLE CPropertyPageImpl : public CDialogImplBaseT< TBase >
{
public:
	PROPSHEETPAGE m_psp;

	operator PROPSHEETPAGE*() { return &m_psp; }

// Construction
	CPropertyPageImpl(_U_STRINGorID title = (LPCTSTR)NULL)
	{
		// initialize PROPSHEETPAGE struct
		memset(&m_psp, 0, sizeof(PROPSHEETPAGE));
		m_psp.dwSize = sizeof(PROPSHEETPAGE);
		m_psp.dwFlags = PSP_USECALLBACK;
		m_psp.hInstance = _Module.GetResourceInstance();
		T* pT = static_cast<T*>(this);
		m_psp.pszTemplate = MAKEINTRESOURCE(pT->IDD);
		m_psp.pfnDlgProc = (DLGPROC)T::StartDialogProc;
		m_psp.pfnCallback = T::PropPageCallback;
		m_psp.lParam = (LPARAM)pT;

		if(title.m_lpstr != NULL)
			SetTitle(title);
	}

	static UINT CALLBACK PropPageCallback(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
	{
		hWnd;	// avoid level 4 warning
		if(uMsg == PSPCB_CREATE)
		{
			ATLASSERT(hWnd == NULL);
			CDialogImplBaseT< TBase >* pPage = (CDialogImplBaseT< TBase >*)(T*)ppsp->lParam;
			_Module.AddCreateWndData(&pPage->m_thunk.cd, pPage);
		}

		return 1;
	}

	HPROPSHEETPAGE Create()
	{
		return ::CreatePropertySheetPage(&m_psp);
	}

// Attributes
	void SetTitle(_U_STRINGorID title)
	{
		m_psp.pszTitle = title.m_lpstr;
		m_psp.dwFlags |= PSP_USETITLE;
	}

#if (_WIN32_IE >= 0x0500)
	void SetHeaderTitle(LPCTSTR lpstrHeaderTitle)
	{
		ATLASSERT(m_hWnd == NULL);	// can't do this after it's created
		m_psp.dwFlags |= PSP_USEHEADERTITLE;
		m_psp.pszHeaderTitle = lpstrHeaderTitle;
	}
	void SetHeaderSubTitle(LPCTSTR lpstrHeaderSubTitle)
	{
		ATLASSERT(m_hWnd == NULL);	// can't do this after it's created
		m_psp.dwFlags |= PSP_USEHEADERSUBTITLE;
		m_psp.pszHeaderSubTitle = lpstrHeaderSubTitle;
	}
#endif //(_WIN32_IE >= 0x0500)

// Operations
	void EnableHelp()
	{
		m_psp.dwFlags |= PSP_HASHELP;
	}

// Message map and handlers
	BEGIN_MSG_MAP(CPropertyPageImpl)
		MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
	END_MSG_MAP()

	// NOTE: Define _WTL_NEW_PAGE_NOTIFY_HANDLERS to use new notification
	// handlers that return direct values without any restrictions
	LRESULT OnNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		NMHDR* pNMHDR = (NMHDR*)lParam;

		// don't handle messages not from the page/sheet itself
		if(pNMHDR->hwndFrom != m_hWnd && pNMHDR->hwndFrom != ::GetParent(m_hWnd))
		{
			bHandled = FALSE;
			return 1;
		}

		T* pT = static_cast<T*>(this);
		LRESULT lResult = 0;
		switch(pNMHDR->code)
		{
#ifdef _WTL_NEW_PAGE_NOTIFY_HANDLERS
		case PSN_SETACTIVE:
			lResult = pT->OnSetActive();
			break;
		case PSN_KILLACTIVE:
			lResult = pT->OnKillActive();
			break;
		case PSN_APPLY:
			lResult = pT->OnApply();
			break;
		case PSN_RESET:
			pT->OnReset();
			break;
		case PSN_QUERYCANCEL:
			lResult = pT->OnQueryCancel();
			break;
		case PSN_WIZNEXT:
			lResult = pT->OnWizardNext();
			break;
		case PSN_WIZBACK:
			lResult = pT->OnWizardBack();
			break;
		case PSN_WIZFINISH:
			lResult = pT->OnWizardFinish();
			break;
		case PSN_HELP:
			pT->OnHelp();
			break;
#if (_WIN32_IE >= 0x0400)
		case PSN_GETOBJECT:
			if(!pT->OnGetObject((LPNMOBJECTNOTIFY)lParam))
				bHandled = FALSE;
			break;
#endif //(_WIN32_IE >= 0x0400)
#if (_WIN32_IE >= 0x0500)
		case PSN_TRANSLATEACCELERATOR:
			{
				LPPSHNOTIFY lpPSHNotify = (LPPSHNOTIFY)lParam;
				lResult = pT->OnTranslateAccelerator((LPMSG)lpPSHNotify->lParam);
			}
			break;
		case PSN_QUERYINITIALFOCUS:
			{
				LPPSHNOTIFY lpPSHNotify = (LPPSHNOTIFY)lParam;
				lResult = (LRESULT)pT->OnQueryInitialFocus((HWND)lpPSHNotify->lParam);
			}
			break;
#endif //(_WIN32_IE >= 0x0500)

#else //!_WTL_NEW_PAGE_NOTIFY_HANDLERS
		case PSN_SETACTIVE:
			lResult = pT->OnSetActive() ? 0 : -1;
			break;
		case PSN_KILLACTIVE:
			lResult = !pT->OnKillActive();
			break;
		case PSN_APPLY:
			lResult = pT->OnApply() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
			break;
		case PSN_RESET:
			pT->OnReset();
			break;
		case PSN_QUERYCANCEL:
			lResult = !pT->OnQueryCancel();
			break;
		case PSN_WIZNEXT:
			lResult = pT->OnWizardNext();
			break;
		case PSN_WIZBACK:
			lResult = pT->OnWizardBack();
			break;
		case PSN_WIZFINISH:
			lResult = !pT->OnWizardFinish();
			break;
		case PSN_HELP:
			pT->OnHelp();
			break;
#if (_WIN32_IE >= 0x0400)
		case PSN_GETOBJECT:
			if(!pT->OnGetObject((LPNMOBJECTNOTIFY)lParam))
				bHandled = FALSE;
			break;
#endif //(_WIN32_IE >= 0x0400)
#if (_WIN32_IE >= 0x0500)
		case PSN_TRANSLATEACCELERATOR:
			{
				LPPSHNOTIFY lpPSHNotify = (LPPSHNOTIFY)lParam;
				lResult = pT->OnTranslateAccelerator((LPMSG)lpPSHNotify->lParam) ? PSNRET_MESSAGEHANDLED : PSNRET_NOERROR;
			}
			break;
		case PSN_QUERYINITIALFOCUS:
			{
				LPPSHNOTIFY lpPSHNotify = (LPPSHNOTIFY)lParam;
				lResult = (LRESULT)pT->OnQueryInitialFocus((HWND)lpPSHNotify->lParam);
			}
			break;
#endif //(_WIN32_IE >= 0x0500)

#endif //!_WTL_NEW_PAGE_NOTIFY_HANDLERS
		default:
			bHandled = FALSE;	// not handled
		}

		return lResult;
	}

// Overridables
	// NOTE: Define _WTL_NEW_PAGE_NOTIFY_HANDLERS to use new notification
	// handlers that return direct values without any restrictions
#ifdef _WTL_NEW_PAGE_NOTIFY_HANDLERS
	int OnSetActive()
	{
		// 0 = allow activate
		// -1 = go back that was active
		// page ID = jump to page
		return 0;
	}
	BOOL OnKillActive()
	{
		// FALSE = allow deactivate
		// TRUE = prevent deactivation
		return FALSE;
	}
	int OnApply()
	{
		// PSNRET_NOERROR = apply OK
		// PSNRET_INVALID = apply not OK, return to this page
		// PSNRET_INVALID_NOCHANGEPAGE = apply not OK, don't change focus
		return PSNRET_NOERROR;
	}
	void OnReset()
	{
	}
	BOOL OnQueryCancel()
	{
		// FALSE = allow cancel
		// TRUE = prevent cancel
		return FALSE;
	}
	int OnWizardBack()
	{
		// 0  = goto previous page
		// -1 = prevent page change
		// >0 = jump to page by dlg ID
		return 0;
	}
	int OnWizardNext()
	{
		// 0  = goto next page
		// -1 = prevent page change
		// >0 = jump to page by dlg ID
		return 0;
	}
	INT_PTR OnWizardFinish()
	{
		// FALSE = allow finish
		// TRUE = prevent finish
		// HWND = prevent finish and set focus to HWND (CommCtrl 5.80 only)
		return FALSE;
	}
	void OnHelp()
	{
	}
#if (_WIN32_IE >= 0x0400)
	BOOL OnGetObject(LPNMOBJECTNOTIFY /*lpObjectNotify*/)
	{
		return FALSE;	// not processed
	}
#endif //(_WIN32_IE >= 0x0400)
#if (_WIN32_IE >= 0x0500)
	int OnTranslateAccelerator(LPMSG /*lpMsg*/)
	{
		// PSNRET_NOERROR - message not handled
		// PSNRET_MESSAGEHANDLED - message handled
		return PSNRET_NOERROR;
	}
	HWND OnQueryInitialFocus(HWND /*hWndFocus*/)
	{
		// NULL = set focus to default control
		// HWND = set focus to HWND
		return NULL;
	}
#endif //(_WIN32_IE >= 0x0500)

#else //!_WTL_NEW_PAGE_NOTIFY_HANDLERS
	BOOL OnSetActive()
	{
		return TRUE;
	}
	BOOL OnKillActive()
	{
		return TRUE;
	}
	BOOL OnApply()
	{
		return TRUE;
	}
	void OnReset()
	{
	}
	BOOL OnQueryCancel()
	{
		return TRUE;    // ok to cancel
	}
	int OnWizardBack()
	{
		// 0  = goto previous page
		// -1 = prevent page change
		// >0 = jump to page by dlg ID
		return 0;
	}
	int OnWizardNext()
	{
		// 0  = goto next page
		// -1 = prevent page change
		// >0 = jump to page by dlg ID
		return 0;
	}
	BOOL OnWizardFinish()
	{
		return TRUE;
	}
	void OnHelp()
	{
	}
#if (_WIN32_IE >= 0x0400)
	BOOL OnGetObject(LPNMOBJECTNOTIFY /*lpObjectNotify*/)
	{
		return FALSE;	// not processed
	}
#endif //(_WIN32_IE >= 0x0400)
#if (_WIN32_IE >= 0x0500)
	BOOL OnTranslateAccelerator(LPMSG /*lpMsg*/)
	{
		return FALSE;	// not translated
	}
	HWND OnQueryInitialFocus(HWND /*hWndFocus*/)
	{
		return NULL;	// default
	}
#endif //(_WIN32_IE >= 0x0500)

#endif //!_WTL_NEW_PAGE_NOTIFY_HANDLERS
};

// for non-customized pages
template <WORD t_wDlgTemplateID>
class CPropertyPage : public CPropertyPageImpl<CPropertyPage<t_wDlgTemplateID> >
{
public:
	enum { IDD = t_wDlgTemplateID };

	CPropertyPage(_U_STRINGorID title = (LPCTSTR)NULL) : CPropertyPageImpl<CPropertyPage>(title)
	{ }

	DECLARE_EMPTY_MSG_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CAxPropertyPageImpl - property page that hosts ActiveX controls

#ifndef _ATL_NO_HOSTING

// Note: You must #include <atlhost.h> to use these classes

template <class T, class TBase = CPropertyPageWindow>
class ATL_NO_VTABLE CAxPropertyPageImpl : public CPropertyPageImpl< T, TBase >
{
public:
// Data members
	HGLOBAL m_hInitData;
	HGLOBAL m_hDlgRes;
	HGLOBAL m_hDlgResSplit;

// Constructor/destructor
	CAxPropertyPageImpl(_U_STRINGorID title = (LPCTSTR)NULL) : 
			CPropertyPageImpl< T, TBase >(title),
			m_hInitData(NULL), m_hDlgRes(NULL), m_hDlgResSplit(NULL)
	{
		T* pT = static_cast<T*>(this);
		pT;	// avoid level 4 warning

		// initialize ActiveX hosting and modify dialog template
		AtlAxWinInit();

		HINSTANCE hInstance = _Module.GetResourceInstance();
		LPCTSTR lpTemplateName = MAKEINTRESOURCE(pT->IDD);
		HRSRC hDlg = ::FindResource(hInstance, lpTemplateName, (LPTSTR)RT_DIALOG);
		if(hDlg != NULL)
		{
			HRSRC hDlgInit = ::FindResource(hInstance, lpTemplateName, (LPTSTR)_ATL_RT_DLGINIT);

			BYTE* pInitData = NULL;
			if(hDlgInit != NULL)
			{
				m_hInitData = ::LoadResource(hInstance, hDlgInit);
				pInitData = (BYTE*)::LockResource(m_hInitData);
			}

			m_hDlgRes = ::LoadResource(hInstance, hDlg);
			DLGTEMPLATE* pDlg = (DLGTEMPLATE*)::LockResource(m_hDlgRes);
			LPCDLGTEMPLATE lpDialogTemplate = _DialogSplitHelper::SplitDialogTemplate(pDlg, pInitData);
			if(lpDialogTemplate != pDlg)
				m_hDlgResSplit = ::GlobalHandle(lpDialogTemplate);

			// set up property page to use in-memory dialog template
			if(lpDialogTemplate != NULL)
			{
				m_psp.dwFlags |= PSP_DLGINDIRECT;
				m_psp.pResource = lpDialogTemplate;
			}
			else
			{
				ATLASSERT(FALSE && _T("CAxPropertyPageImpl - ActiveX initializtion failed!"));
			}
		}
		else
		{
			ATLASSERT(FALSE && _T("CAxPropertyPageImpl - Cannot find dialog template!"));
		}
	}

	~CAxPropertyPageImpl()
	{
		if(m_hInitData != NULL)
		{
			UnlockResource(m_hInitData);
			::FreeResource(m_hInitData);
		}
		if(m_hDlgRes != NULL)
		{
			UnlockResource(m_hDlgRes);
			::FreeResource(m_hDlgRes);
		}
		if(m_hDlgResSplit != NULL)
		{
			::GlobalFree(m_hDlgResSplit);
		}
	}

// Methods
	// call this one to handle keyboard message for ActiveX controls
	BOOL PreTranslateMessage(LPMSG pMsg)
	{
		if ((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
		   (pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
			return FALSE;
		// find a direct child of the dialog from the window that has focus
		HWND hWndCtl = ::GetFocus();
		if (IsChild(hWndCtl) && ::GetParent(hWndCtl) != m_hWnd)
		{
			do
			{
				hWndCtl = ::GetParent(hWndCtl);
			}
			while (::GetParent(hWndCtl) != m_hWnd);
		}
		// give controls a chance to translate this message
		return (BOOL)::SendMessage(hWndCtl, WM_FORWARDMSG, 0, (LPARAM)pMsg);
	}

// Overridables
#if (_WIN32_IE >= 0x0500)
	// new default implementation for ActiveX hosting pages
	BOOL OnTranslateAccelerator(LPMSG lpMsg)
	{
		T* pT = static_cast<T*>(this);
		return pT->PreTranslateMessage(lpMsg);
	}
#endif //(_WIN32_IE >= 0x0500)
};

// for non-customized pages
template <WORD t_wDlgTemplateID>
class CAxPropertyPage : public CAxPropertyPageImpl<CAxPropertyPage<t_wDlgTemplateID> >
{
public:
	enum { IDD = t_wDlgTemplateID };

	CAxPropertyPage(_U_STRINGorID title = (LPCTSTR)NULL) : CAxPropertyPageImpl<CAxPropertyPage>(title)
	{ }

#if (_WIN32_IE >= 0x0500)
	// not empty so we handle accelerators
	BEGIN_MSG_MAP(CAxPropertyPage)
		CHAIN_MSG_MAP(CAxPropertyPageImpl<CAxPropertyPage<t_wDlgTemplateID> >)
	END_MSG_MAP()
#else //!(_WIN32_IE >= 0x0500)
	DECLARE_EMPTY_MSG_MAP()
#endif //!(_WIN32_IE >= 0x0500)
};

#endif //_ATL_NO_HOSTING

}; //namespace WTL

#endif // __ATLDLGS_H__
