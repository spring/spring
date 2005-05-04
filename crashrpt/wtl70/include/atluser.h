// Windows Template Library - WTL version 7.0
// Copyright (C) 1997-2002 Microsoft Corporation
// All rights reserved.
//
// This file is a part of the Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.

#ifndef __ATLUSER_H__
#define __ATLUSER_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error atluser.h requires atlbase.h to be included first
#endif


/////////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CMenuItemInfo
// CMenuT<t_bManaged>


namespace WTL
{

/////////////////////////////////////////////////////////////////////////////
// AtlMessageBox - accepts both memory and resource based strings

inline int AtlMessageBox(HWND hWndOwner, _U_STRINGorID message, _U_STRINGorID title = (LPCTSTR)NULL, UINT uType = MB_OK | MB_ICONINFORMATION)
{
	ATLASSERT(hWndOwner == NULL || ::IsWindow(hWndOwner));

	LPTSTR lpstrMessage = NULL;
	if(IS_INTRESOURCE(message.m_lpstr))
	{
		for(int nLen = 256; ; nLen *= 2)
		{
			ATLTRY(lpstrMessage = new TCHAR[nLen]);
			if(lpstrMessage == NULL)
			{
				ATLASSERT(FALSE);
				return 0;
			}
			int nRes = ::LoadString(_pModule->GetResourceInstance(), LOWORD(message.m_lpstr), lpstrMessage, nLen);
			if(nRes < nLen - 1)
				break;
			delete [] lpstrMessage;
			lpstrMessage = NULL;
		}

		message.m_lpstr = lpstrMessage;
	}

	LPTSTR lpstrTitle = NULL;
	if(IS_INTRESOURCE(title.m_lpstr) && LOWORD(title.m_lpstr) != 0)
	{
		for(int nLen = 256; ; nLen *= 2)
		{
			ATLTRY(lpstrTitle = new TCHAR[nLen]);
			if(lpstrTitle == NULL)
			{
				ATLASSERT(FALSE);
				return 0;
			}
			int nRes = ::LoadString(_pModule->GetResourceInstance(), LOWORD(title.m_lpstr), lpstrTitle, nLen);
			if(nRes < nLen - 1)
				break;
			delete [] lpstrTitle;
			lpstrTitle = NULL;
		}

		title.m_lpstr = lpstrTitle;
	}

	int nRet = ::MessageBox(hWndOwner, message.m_lpstr, title.m_lpstr, uType);

	delete [] lpstrMessage;
	delete [] lpstrTitle;

	return nRet;
}


/////////////////////////////////////////////////////////////////////////////
// CMenu

class CMenuItemInfo : public MENUITEMINFO
{
public:
	CMenuItemInfo()
	{
		memset(this, 0, sizeof(MENUITEMINFO));
		cbSize = sizeof(MENUITEMINFO);
	}
};


// forward declarations
template <bool t_bManaged> class CMenuT;
typedef CMenuT<false>		CMenuHandle;
typedef CMenuT<true>		CMenu;


template <bool t_bManaged>
class CMenuT
{
public:
// Data members
	HMENU m_hMenu;

// Constructor/destructor/operators
	CMenuT(HMENU hMenu = NULL) : m_hMenu(hMenu)
	{ }

	~CMenuT()
	{
		if(t_bManaged && m_hMenu != NULL)
			DestroyMenu();
	}

	CMenuT<t_bManaged>& operator=(HMENU hMenu)
	{
		m_hMenu = hMenu;
		return *this;
	}

	void Attach(HMENU hMenuNew)
	{
		ATLASSERT(::IsMenu(hMenuNew));
		if(t_bManaged && m_hMenu != NULL)
			::DestroyMenu(m_hMenu);
		m_hMenu = hMenuNew;
	}

	HMENU Detach()
	{
		HMENU hMenu = m_hMenu;
		m_hMenu = NULL;
		return hMenu;
	}

	operator HMENU() const { return m_hMenu; }

	BOOL IsMenu() const
	{
		return ::IsMenu(m_hMenu);
	}

// Create and load methods
	BOOL CreateMenu()
	{
		ATLASSERT(m_hMenu == NULL);
		m_hMenu = ::CreateMenu();
		return (m_hMenu != NULL) ? TRUE : FALSE;
	}
	BOOL CreatePopupMenu()
	{
		ATLASSERT(m_hMenu == NULL);
		m_hMenu = ::CreatePopupMenu();
		return (m_hMenu != NULL) ? TRUE : FALSE;
	}
	BOOL LoadMenu(_U_STRINGorID menu)
	{
		ATLASSERT(m_hMenu == NULL);
		m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), menu.m_lpstr);
		return (m_hMenu != NULL) ? TRUE : FALSE;
	}
	BOOL LoadMenuIndirect(const void* lpMenuTemplate)
	{
		ATLASSERT(m_hMenu == NULL);
		m_hMenu = ::LoadMenuIndirect(lpMenuTemplate);
		return (m_hMenu != NULL) ? TRUE : FALSE;
	}
	BOOL DestroyMenu()
	{
		if (m_hMenu == NULL)
			return FALSE;
		BOOL bRet = ::DestroyMenu(m_hMenu);
		if(bRet)
			m_hMenu = NULL;
		return bRet;
	}

// Menu Operations
	BOOL DeleteMenu(UINT nPosition, UINT nFlags)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::DeleteMenu(m_hMenu, nPosition, nFlags);
	}
	BOOL TrackPopupMenu(UINT nFlags, int x, int y, HWND hWnd, LPCRECT lpRect = NULL)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::TrackPopupMenu(m_hMenu, nFlags, x, y, 0, hWnd, lpRect);
	}
	BOOL TrackPopupMenuEx(UINT uFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm = NULL)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::TrackPopupMenuEx(m_hMenu, uFlags, x, y, hWnd, lptpm);
	}

#if (WINVER >= 0x0500)
	BOOL GetMenuInfo(LPMENUINFO lpMenuInfo) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuInfo(m_hMenu, lpMenuInfo);
	}
	BOOL SetMenuInfo(LPCMENUINFO lpMenuInfo)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::SetMenuInfo(m_hMenu, lpMenuInfo);
	}
#endif //(WINVER >= 0x0500)

// Menu Item Operations
	BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = NULL)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::AppendMenu(m_hMenu, nFlags, nIDNewItem, lpszNewItem);
	}
	BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem, HBITMAP hBmp)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::AppendMenu(m_hMenu, nFlags | MF_BITMAP, nIDNewItem, (LPCTSTR)hBmp);
	}
	UINT CheckMenuItem(UINT nIDCheckItem, UINT nCheck)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return (UINT)::CheckMenuItem(m_hMenu, nIDCheckItem, nCheck);
	}
	UINT EnableMenuItem(UINT nIDEnableItem, UINT nEnable)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::EnableMenuItem(m_hMenu, nIDEnableItem, nEnable);
	}
	BOOL HiliteMenuItem(HWND hWnd, UINT uIDHiliteItem, UINT uHilite)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::HiliteMenuItem(hWnd, m_hMenu, uIDHiliteItem, uHilite);
	}
	int GetMenuItemCount() const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuItemCount(m_hMenu);
	}
	UINT GetMenuItemID(int nPos) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuItemID(m_hMenu, nPos);
	}
	UINT GetMenuState(UINT nID, UINT nFlags) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuState(m_hMenu, nID, nFlags);
	}
	int GetMenuString(UINT nIDItem, LPTSTR lpString, int nMaxCount, UINT nFlags) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuString(m_hMenu, nIDItem, lpString, nMaxCount, nFlags);
	}
	int GetMenuStringLen(UINT nIDItem, UINT nFlags) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuString(m_hMenu, nIDItem, NULL, 0, nFlags);
	}
#ifndef _ATL_NO_COM
	BOOL GetMenuString(UINT nIDItem, BSTR& bstrText, UINT nFlags) const
	{
		USES_CONVERSION;
		ATLASSERT(::IsMenu(m_hMenu));
		ATLASSERT(bstrText == NULL);

		int nLen = GetMenuStringLen(nIDItem, nFlags);
		if(nLen == 0)
		{
			bstrText = ::SysAllocString(OLESTR(""));
			return (bstrText != NULL) ? TRUE : FALSE;
		}

		nLen++;		// increment to include terminating NULL char
		LPTSTR lpszText = (LPTSTR)_alloca((nLen) * sizeof(TCHAR));

		if(!GetMenuString(nIDItem, lpszText, nLen, nFlags))
			return FALSE;

		bstrText = ::SysAllocString(T2OLE(lpszText));
		return (bstrText != NULL) ? TRUE : FALSE;
	}
#endif //!_ATL_NO_COM
#if defined(_WTL_USE_CSTRING) || defined(__ATLSTR_H__)
	int GetMenuString(UINT nIDItem, CString& strText, UINT nFlags) const
	{
		ATLASSERT(::IsMenu(m_hMenu));

		int nLen = GetMenuStringLen(nIDItem, nFlags);
		if(nLen == 0)
			return 0;

		nLen++;		// increment to include terminating NULL char
		LPTSTR lpstr = strText.GetBufferSetLength(nLen);
		if(lpstr == NULL)
			return 0;
		int nRet = GetMenuString(nIDItem, lpstr, nLen, nFlags);
		strText.ReleaseBuffer();
		return nRet;
	}
#endif //defined(_WTL_USE_CSTRING) || defined(__ATLSTR_H__)
	CMenuHandle GetSubMenu(int nPos) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return CMenuHandle(::GetSubMenu(m_hMenu, nPos));
	}
	BOOL InsertMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = NULL)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::InsertMenu(m_hMenu, nPosition, nFlags, nIDNewItem, lpszNewItem);
	}
	BOOL InsertMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem, HBITMAP hBmp)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::InsertMenu(m_hMenu, nPosition, nFlags | MF_BITMAP, nIDNewItem, (LPCTSTR)hBmp);
	}
	BOOL ModifyMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = NULL)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::ModifyMenu(m_hMenu, nPosition, nFlags, nIDNewItem, lpszNewItem);
	}
	BOOL ModifyMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem, HBITMAP hBmp)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::ModifyMenu(m_hMenu, nPosition, nFlags | MF_BITMAP, nIDNewItem, (LPCTSTR)hBmp);
	}
	BOOL RemoveMenu(UINT nPosition, UINT nFlags)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::RemoveMenu(m_hMenu, nPosition, nFlags);
	}
	BOOL SetMenuItemBitmaps(UINT nPosition, UINT nFlags, HBITMAP hBmpUnchecked, HBITMAP hBmpChecked)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::SetMenuItemBitmaps(m_hMenu, nPosition, nFlags, hBmpUnchecked, hBmpChecked);
	}
	BOOL CheckMenuRadioItem(UINT nIDFirst, UINT nIDLast, UINT nIDItem, UINT nFlags)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::CheckMenuRadioItem(m_hMenu, nIDFirst, nIDLast, nIDItem, nFlags);
	}

	BOOL GetMenuItemInfo(UINT uItem, BOOL bByPosition, LPMENUITEMINFO lpmii) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return (BOOL)::GetMenuItemInfo(m_hMenu, uItem, bByPosition, lpmii);
	}
	BOOL SetMenuItemInfo(UINT uItem, BOOL bByPosition, LPMENUITEMINFO lpmii)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return (BOOL)::SetMenuItemInfo(m_hMenu, uItem, bByPosition, lpmii);
	}
	BOOL InsertMenuItem(UINT uItem, BOOL bByPosition, LPMENUITEMINFO lpmii)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return (BOOL)::InsertMenuItem(m_hMenu, uItem, bByPosition, lpmii);
	}

	UINT GetMenuDefaultItem(BOOL bByPosition = FALSE, UINT uFlags = 0U) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuDefaultItem(m_hMenu, (UINT)bByPosition, uFlags);
	}
	BOOL SetMenuDefaultItem(UINT uItem = (UINT)-1,  BOOL bByPosition = FALSE)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::SetMenuDefaultItem(m_hMenu, uItem, (UINT)bByPosition);
	}
	BOOL GetMenuItemRect(HWND hWnd, UINT uItem, LPRECT lprcItem) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuItemRect(hWnd, m_hMenu, uItem, lprcItem);
	}
	int MenuItemFromPoint(HWND hWnd, POINT point) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::MenuItemFromPoint(hWnd, m_hMenu, point);
	}

// Context Help Functions
	BOOL SetMenuContextHelpId(DWORD dwContextHelpId)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::SetMenuContextHelpId(m_hMenu, dwContextHelpId);
	}
	DWORD GetMenuContextHelpId() const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuContextHelpId(m_hMenu);
	}
};

}; //namespace WTL

#endif // __ATLUSER_H__
