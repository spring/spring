// Windows Template Library - WTL version 7.0
// Copyright (C) 1997-2002 Microsoft Corporation
// All rights reserved.
//
// This file is a part of the Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.

#ifndef __ATLSCRL_H__
#define __ATLSCRL_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
	#error atlscrl.h requires atlapp.h to be included first
#endif

#ifndef __ATLWIN_H__
	#error atlscrl.h requires atlwin.h to be included first
#endif

#if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
#include <zmouse.h>
#endif //!((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))


/////////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CScrollImpl<T>
// CScrollWindowImpl<T, TBase, TWinTraits>
// CMapScrollImpl<T>
// CMapScrollWindowImpl<T, TBase, TWinTraits>
// CFSBWindowT<TBase>


namespace WTL
{

/////////////////////////////////////////////////////////////////////////////
// CScrollImpl - Provides scrolling support to any window

// Scroll extended styles
#define SCRL_SCROLLCHILDREN	0x00000001
#define SCRL_ERASEBACKGROUND	0x00000002
#define SCRL_NOTHUMBTRACKING	0x00000004
#if (WINVER >= 0x0500)
#define SCRL_SMOOTHSCROLL	0x00000008
#endif //(WINVER >= 0x0500)


template <class T>
class CScrollImpl
{
public:
	enum { uSCROLL_FLAGS = SW_INVALIDATE };

	POINT m_ptOffset;
	SIZE m_sizeAll;
	SIZE m_sizeLine;
	SIZE m_sizePage;
	SIZE m_sizeClient;
	int m_zDelta;			// current wheel value
	int m_nWheelLines;		// number of lines to scroll on wheel
#if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
	UINT m_uMsgMouseWheel;		// MSH_MOUSEWHEEL
	// Note that this message must be forwarded from a top level window
#endif //!((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
	UINT m_uScrollFlags;
	DWORD m_dwExtendedStyle;	// scroll specific extended styles

// Constructor
	CScrollImpl() : m_zDelta(0), m_nWheelLines(3), m_uScrollFlags(0U), m_dwExtendedStyle(0)
	{
		m_ptOffset.x = 0;
		m_ptOffset.y = 0;
		m_sizeAll.cx = 0;
		m_sizeAll.cy = 0;
		m_sizePage.cx = 0;
		m_sizePage.cy = 0;
		m_sizeLine.cx = 0;
		m_sizeLine.cy = 0;
		m_sizeClient.cx = 0;
		m_sizeClient.cy = 0;

		SetScrollExtendedStyle(SCRL_SCROLLCHILDREN | SCRL_ERASEBACKGROUND);
	}

// Attributes & Operations
	DWORD GetScrollExtendedStyle() const
	{
		return m_dwExtendedStyle;
	}

	DWORD SetScrollExtendedStyle(DWORD dwExtendedStyle, DWORD dwMask = 0)
	{
		DWORD dwPrevStyle = m_dwExtendedStyle;
		if(dwMask == 0)
			m_dwExtendedStyle = dwExtendedStyle;
		else
			m_dwExtendedStyle = (m_dwExtendedStyle & ~dwMask) | (dwExtendedStyle & dwMask);
		// cache scroll flags
		T* pT = static_cast<T*>(this);
		pT;	// avoid level 4 warning
		m_uScrollFlags = pT->uSCROLL_FLAGS | (IsScrollingChildren() ? SW_SCROLLCHILDREN : 0) | (IsErasingBackground() ? SW_ERASE : 0);
#if (WINVER >= 0x0500)
		m_uScrollFlags |= (IsSmoothScroll() ? SW_SMOOTHSCROLL : 0);
#endif //(WINVER >= 0x0500)
		return dwPrevStyle;
	}

	// offset operations
	void SetScrollOffset(int x, int y, BOOL bRedraw = TRUE)
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));

		m_ptOffset.x = x;
		m_ptOffset.y = y;

		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;

		si.nPos = m_ptOffset.x;
		pT->SetScrollInfo(SB_HORZ, &si, bRedraw);

		si.nPos = m_ptOffset.y;
		pT->SetScrollInfo(SB_VERT, &si, bRedraw);

		if(bRedraw)
			pT->Invalidate();
	}
	void SetScrollOffset(POINT ptOffset, BOOL bRedraw = TRUE)
	{
		SetScrollOffset(ptOffset.x, ptOffset.y, bRedraw);
	}
	void GetScrollOffset(POINT& ptOffset) const
	{
		ptOffset = m_ptOffset;
	}

	// size operations
	void SetScrollSize(int cx, int cy, BOOL bRedraw = TRUE)
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));

		m_sizeAll.cx = cx;
		m_sizeAll.cy = cy;

		m_ptOffset.x = 0;
		m_ptOffset.y = 0;

		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
		si.nMin = 0;

		si.nMax = m_sizeAll.cx - 1;
		si.nPage = m_sizeClient.cx;
		si.nPos = m_ptOffset.x;
		pT->SetScrollInfo(SB_HORZ, &si, bRedraw);

		si.nMax = m_sizeAll.cy - 1;
		si.nPage = m_sizeClient.cy;
		si.nPos = m_ptOffset.y;
		pT->SetScrollInfo(SB_VERT, &si, bRedraw);

		SetScrollLine(0, 0);
		SetScrollPage(0, 0);

		if(bRedraw)
			pT->Invalidate();
	}
	void SetScrollSize(SIZE size, BOOL bRedraw = TRUE)
	{
		SetScrollSize(size.cx, size.cy, bRedraw);
	}
	void GetScrollSize(SIZE& sizeWnd) const
	{
		sizeWnd = m_sizeAll;
	}

	// line operations
	void SetScrollLine(int cxLine, int cyLine)
	{
		ATLASSERT(cxLine >= 0 && cyLine >= 0);
		ATLASSERT(m_sizeAll.cx != 0 && m_sizeAll.cy != 0);

		m_sizeLine.cx = CalcLineOrPage(cxLine, m_sizeAll.cx, 100);
		m_sizeLine.cy = CalcLineOrPage(cyLine, m_sizeAll.cy, 100);
	}
	void SetScrollLine(SIZE sizeLine)
	{
		SetScrollLine(sizeLine.cx, sizeLine.cy);
	}
	void GetScrollLine(SIZE& sizeLine) const
	{
		sizeLine = m_sizeLine;
	}

	// page operations
	void SetScrollPage(int cxPage, int cyPage)
	{
		ATLASSERT(cxPage >= 0 && cyPage >= 0);
		ATLASSERT(m_sizeAll.cx != 0 && m_sizeAll.cy != 0);

		m_sizePage.cx = CalcLineOrPage(cxPage, m_sizeAll.cx, 10);
		m_sizePage.cy = CalcLineOrPage(cyPage, m_sizeAll.cy, 10);
	}
	void SetScrollPage(SIZE sizePage)
	{
		SetScrollPage(sizePage.cx, sizePage.cy);
	}
	void GetScrollPage(SIZE& sizePage) const
	{
		sizePage = m_sizePage;
	}

	// commands
	void ScrollLineDown()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_VERT, SB_LINEDOWN, (int&)m_ptOffset.y, m_sizeAll.cy, m_sizePage.cy, m_sizeLine.cy);
	}

	void ScrollLineUp()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_VERT, SB_LINEUP, (int&)m_ptOffset.y, m_sizeAll.cy, m_sizePage.cy, m_sizeLine.cy);
	}

	void ScrollPageDown()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_VERT, SB_PAGEDOWN, (int&)m_ptOffset.y, m_sizeAll.cy, m_sizePage.cy, m_sizeLine.cy);
	}

	void ScrollPageUp()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_VERT, SB_PAGEUP, (int&)m_ptOffset.y, m_sizeAll.cy, m_sizePage.cy, m_sizeLine.cy);
	}

	void ScrollTop()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_VERT, SB_TOP, (int&)m_ptOffset.y, m_sizeAll.cy, m_sizePage.cy, m_sizeLine.cy);
	}

	void ScrollBottom()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_VERT, SB_BOTTOM, (int&)m_ptOffset.y, m_sizeAll.cy, m_sizePage.cy, m_sizeLine.cy);
	}

	void ScrollLineRight()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_HORZ, SB_LINEDOWN, (int&)m_ptOffset.x, m_sizeAll.cx, m_sizePage.cx, m_sizeLine.cx);
	}

	void ScrollLineLeft()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_HORZ, SB_LINEUP, (int&)m_ptOffset.x, m_sizeAll.cx, m_sizePage.cx, m_sizeLine.cx);
	}

	void ScrollPageRight()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_HORZ, SB_PAGEDOWN, (int&)m_ptOffset.x, m_sizeAll.cx, m_sizePage.cx, m_sizeLine.cx);
	}

	void ScrollPageLeft()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_HORZ, SB_PAGEUP, (int&)m_ptOffset.x, m_sizeAll.cx, m_sizePage.cx, m_sizeLine.cx);
	}

	void ScrollAllLeft()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_HORZ, SB_TOP, (int&)m_ptOffset.x, m_sizeAll.cx, m_sizePage.cx, m_sizeLine.cx);
	}

	void ScrollAllRight()
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_HORZ, SB_BOTTOM, (int&)m_ptOffset.x, m_sizeAll.cx, m_sizePage.cx, m_sizeLine.cx);
	}

	BEGIN_MSG_MAP(CScrollImpl< T >)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
		MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
#if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
		MESSAGE_HANDLER(m_uMsgMouseWheel, OnMouseWheel)
#endif //(_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
		MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
	// standard scroll commands
	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_SCROLL_UP, OnScrollUp)
		COMMAND_ID_HANDLER(ID_SCROLL_DOWN, OnScrollDown)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_UP, OnScrollPageUp)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_DOWN, OnScrollPageDown)
		COMMAND_ID_HANDLER(ID_SCROLL_TOP, OnScrollTop)
		COMMAND_ID_HANDLER(ID_SCROLL_BOTTOM, OnScrollBottom)
		COMMAND_ID_HANDLER(ID_SCROLL_LEFT, OnScrollLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_RIGHT, OnScrollRight)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_LEFT, OnScrollPageLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_RIGHT, OnScrollPageRight)
		COMMAND_ID_HANDLER(ID_SCROLL_ALL_LEFT, OnScrollAllLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_ALL_RIGHT, OnScrollAllRight)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		GetSystemSettings();
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnVScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_VERT, (int)(short)LOWORD(wParam), (int&)m_ptOffset.y, m_sizeAll.cy, m_sizePage.cy, m_sizeLine.cy);
		return 0;
	}

	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		pT->DoScroll(SB_HORZ, (int)(short)LOWORD(wParam), (int&)m_ptOffset.x, m_sizeAll.cx, m_sizePage.cx, m_sizeLine.cx);
		return 0;
	}

	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));

#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
		uMsg;
		int zDelta = (int)(short)HIWORD(wParam);
#else
		int zDelta = (uMsg == WM_MOUSEWHEEL) ? (int)(short)HIWORD(wParam) : (int)wParam;
#endif //!((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
		int nScrollCode = (m_nWheelLines == WHEEL_PAGESCROLL) ? ((zDelta > 0) ? SB_PAGEUP : SB_PAGEDOWN) : ((zDelta > 0) ? SB_LINEUP : SB_LINEDOWN);
		m_zDelta += zDelta;		// cumulative
		int zTotal = (m_nWheelLines == WHEEL_PAGESCROLL) ? abs(m_zDelta) : abs(m_zDelta) * m_nWheelLines;
		if((pT->GetStyle() & WS_VSCROLL) != 0)
		{
			for(short i = 0; i < zTotal; i += WHEEL_DELTA)
			{
				pT->DoScroll(SB_VERT, nScrollCode, (int&)m_ptOffset.y, m_sizeAll.cy, m_sizePage.cy, m_sizeLine.cy);
				pT->UpdateWindow();
			}
		}
		else		// can't scroll vertically, scroll horizontally
		{
			for(short i = 0; i < zTotal; i += WHEEL_DELTA)
			{
				pT->DoScroll(SB_HORZ, nScrollCode, (int&)m_ptOffset.x, m_sizeAll.cx, m_sizePage.cx, m_sizeLine.cx);
				pT->UpdateWindow();
			}
		}
		int nSteps = m_zDelta / WHEEL_DELTA;
		m_zDelta -= nSteps * WHEEL_DELTA;

		return 0;
	}

	LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		GetSystemSettings();
		return 0;
	}

	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));

		m_sizeClient.cx = GET_X_LPARAM(lParam);
		m_sizeClient.cy = GET_Y_LPARAM(lParam);

		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_POS;

		si.nPage = m_sizeClient.cx;
		si.nPos = m_ptOffset.x;
		pT->SetScrollInfo(SB_HORZ, &si, FALSE);

		si.nPage = m_sizeClient.cy;
		si.nPos = m_ptOffset.y;
		pT->SetScrollInfo(SB_VERT, &si, FALSE);

		bool bUpdate = false;
		int cxMax = m_sizeAll.cx - m_sizeClient.cx;
		int cyMax = m_sizeAll.cy - m_sizeClient.cy;
		int x = m_ptOffset.x;
		int y = m_ptOffset.y;
		if(m_ptOffset.x > cxMax)
		{
			bUpdate = true;
			x = (cxMax >= 0) ? cxMax : 0;
		}
		if(m_ptOffset.y > cyMax)
		{
			bUpdate = true;
			y = (cyMax >= 0) ? cyMax : 0;
		}
		if(bUpdate)
		{
			pT->ScrollWindowEx(m_ptOffset.x - x, m_ptOffset.y - y, m_uScrollFlags);
			SetScrollOffset(x, y, FALSE);
		}

		bHandled = FALSE;
		return 1;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		if(wParam != NULL)
		{
			CDCHandle dc = (HDC)wParam;
			dc.SetViewportOrg(-m_ptOffset.x, -m_ptOffset.y);
			pT->DoPaint(dc);
		}
		else
		{
			CPaintDC dc(pT->m_hWnd);
			dc.SetViewportOrg(-m_ptOffset.x, -m_ptOffset.y);
			pT->DoPaint(dc.m_hDC);
		}
		return 0;
	}

	// scrolling handlers
	LRESULT OnScrollUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollLineUp();
		return 0;
	}
	LRESULT OnScrollDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollLineDown();
		return 0;
	}
	LRESULT OnScrollPageUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollPageUp();
		return 0;
	}
	LRESULT OnScrollPageDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollPageDown();
		return 0;
	}
	LRESULT OnScrollTop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollTop();
		return 0;
	}
	LRESULT OnScrollBottom(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollBottom();
		return 0;
	}
	LRESULT OnScrollLeft(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollLineLeft();
		return 0;
	}
	LRESULT OnScrollRight(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollLineRight();
		return 0;
	}
	LRESULT OnScrollPageLeft(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollPageLeft();
		return 0;
	}
	LRESULT OnScrollPageRight(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollPageRight();
		return 0;
	}
	LRESULT OnScrollAllLeft(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollAllLeft();
		return 0;
	}
	LRESULT OnScrollAllRight(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		ScrollAllRight();
		return 0;
	}

// Overrideables
	void DoPaint(CDCHandle /*dc*/)
	{
		// must be implemented in a derived class
		ATLASSERT(FALSE);
	}

// Implementation
	void DoScroll(int nType, int nScrollCode, int& cxyOffset, int cxySizeAll, int cxySizePage, int cxySizeLine)
	{
		T* pT = static_cast<T*>(this);
		RECT rect;
		pT->GetClientRect(&rect);
		int cxyClient = (nType == SB_VERT) ? rect.bottom : rect.right;
		int cxyMax = cxySizeAll - cxyClient;

		if(cxyMax < 0)		// can't scroll, client area is bigger
			return;

		BOOL bUpdate = TRUE;
		int cxyScroll = 0;

		switch(nScrollCode)
		{
		case SB_TOP:		// top or all left
			cxyScroll = cxyOffset;
			cxyOffset = 0;
			break;
		case SB_BOTTOM:		// bottom or all right
			cxyScroll = cxyOffset - cxyMax;
			cxyOffset = cxyMax;
			break;
		case SB_LINEUP:		// line up or line left
			if(cxyOffset >= cxySizeLine)
			{
				cxyScroll = cxySizeLine;
				cxyOffset -= cxySizeLine;
			}
			else
			{
				cxyScroll = cxyOffset;
				cxyOffset = 0;
			}
			break;
		case SB_LINEDOWN:	// line down or line right
			if(cxyOffset < cxyMax - cxySizeLine)
			{
				cxyScroll = -cxySizeLine;
				cxyOffset += cxySizeLine;
			}
			else
			{
				cxyScroll = cxyOffset - cxyMax;
				cxyOffset = cxyMax;
			}
			break;
		case SB_PAGEUP:		// page up or page left
			if(cxyOffset >= cxySizePage)
			{
				cxyScroll = cxySizePage;
				cxyOffset -= cxySizePage;
			}
			else
			{
				cxyScroll = cxyOffset;
				cxyOffset = 0;
			}
			break;
		case SB_PAGEDOWN:	// page down or page right
			if(cxyOffset < cxyMax - cxySizePage)
			{
				cxyScroll = -cxySizePage;
				cxyOffset += cxySizePage;
			}
			else
			{
				cxyScroll = cxyOffset - cxyMax;
				cxyOffset = cxyMax;
			}
			break;
		case SB_THUMBTRACK:
			if(IsNoThumbTracking())
				break;
			// else fall through
		case SB_THUMBPOSITION:
			{
				SCROLLINFO si;
				si.cbSize = sizeof(SCROLLINFO);
				si.fMask = SIF_TRACKPOS;
				if(pT->GetScrollInfo(nType, &si))
				{
					cxyScroll = cxyOffset - si.nTrackPos;
					cxyOffset = si.nTrackPos;
				}
			}
			break;
		case SB_ENDSCROLL:
		default:
			bUpdate = FALSE;
			break;
		}

		if(bUpdate && cxyScroll != 0)
		{
			pT->SetScrollPos(nType, cxyOffset, TRUE);
			if(nType == SB_VERT)
				pT->ScrollWindowEx(0, cxyScroll, m_uScrollFlags);
			else
				pT->ScrollWindowEx(cxyScroll, 0, m_uScrollFlags);
		}
	}
	int CalcLineOrPage(int nVal, int nMax, int nDiv)
	{
		if(nVal == 0)
		{
			nVal = nMax / nDiv;
			if(nVal < 1)
				nVal = 1;
		}
		else if(nVal > nMax)
			nVal = nMax;

		return nVal;
	}
	void GetSystemSettings()
	{
#ifndef SPI_GETWHEELSCROLLLINES
		const UINT SPI_GETWHEELSCROLLLINES = 104;
#endif //!SPI_GETWHEELSCROLLLINES
		::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &m_nWheelLines, 0);

#if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
		if(m_uMsgMouseWheel != 0)
			m_uMsgMouseWheel = ::RegisterWindowMessage(MSH_MOUSEWHEEL);

		HWND hWndWheel = FindWindow(MSH_WHEELMODULE_CLASS, MSH_WHEELMODULE_TITLE);
		if(::IsWindow(hWndWheel))
		{
			UINT uMsgScrollLines = ::RegisterWindowMessage(MSH_SCROLL_LINES);
			if(uMsgScrollLines != 0)
				m_nWheelLines = ::SendMessage(hWndWheel, uMsgScrollLines, 0, 0L);
		}
#endif //!((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
	}
	bool IsScrollingChildren() const
	{
		return (m_dwExtendedStyle & SCRL_SCROLLCHILDREN) != 0;
	}
	bool IsErasingBackground() const
	{
		return (m_dwExtendedStyle & SCRL_ERASEBACKGROUND) != 0;
	}
	bool IsNoThumbTracking() const
	{
		return (m_dwExtendedStyle & SCRL_NOTHUMBTRACKING) != 0;
	}
#if (WINVER >= 0x0500)
	bool IsSmoothScroll() const
	{
		return (m_dwExtendedStyle & SCRL_SMOOTHSCROLL) != 0;
	}
#endif //(WINVER >= 0x0500)
};


/////////////////////////////////////////////////////////////////////////////
// CScrollWindowImpl - Implements a scrollable window

template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CScrollWindowImpl : public CWindowImpl<T, TBase, TWinTraits>, public CScrollImpl< T >
{
public:
	BEGIN_MSG_MAP(CScrollImpl< T >)
		MESSAGE_HANDLER(WM_VSCROLL, CScrollImpl< T >::OnVScroll)
		MESSAGE_HANDLER(WM_HSCROLL, CScrollImpl< T >::OnHScroll)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, CScrollImpl< T >::OnMouseWheel)
#if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
		MESSAGE_HANDLER(m_uMsgMouseWheel, CScrollImpl< T >::OnMouseWheel)
#endif //(_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
		MESSAGE_HANDLER(WM_SETTINGCHANGE, CScrollImpl< T >::OnSettingChange)
		MESSAGE_HANDLER(WM_SIZE, CScrollImpl< T >::OnSize)
		MESSAGE_HANDLER(WM_PAINT, CScrollImpl< T >::OnPaint)
		MESSAGE_HANDLER(WM_PRINTCLIENT, CScrollImpl< T >::OnPaint)
	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_SCROLL_UP, CScrollImpl< T >::OnScrollUp)
		COMMAND_ID_HANDLER(ID_SCROLL_DOWN, CScrollImpl< T >::OnScrollDown)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_UP, CScrollImpl< T >::OnScrollPageUp)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_DOWN, CScrollImpl< T >::OnScrollPageDown)
		COMMAND_ID_HANDLER(ID_SCROLL_TOP, CScrollImpl< T >::OnScrollTop)
		COMMAND_ID_HANDLER(ID_SCROLL_BOTTOM, CScrollImpl< T >::OnScrollBottom)
		COMMAND_ID_HANDLER(ID_SCROLL_LEFT, CScrollImpl< T >::OnScrollLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_RIGHT, CScrollImpl< T >::OnScrollRight)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_LEFT, CScrollImpl< T >::OnScrollPageLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_RIGHT, CScrollImpl< T >::OnScrollPageRight)
		COMMAND_ID_HANDLER(ID_SCROLL_ALL_LEFT, CScrollImpl< T >::OnScrollAllLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_ALL_RIGHT, CScrollImpl< T >::OnScrollAllRight)
	END_MSG_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CMapScrollImpl - Provides mapping and scrolling support to any window

template <class T>
class CMapScrollImpl : public CScrollImpl< T >
{
public:
	int m_nMapMode;
	RECT m_rectLogAll;
	SIZE m_sizeLogLine;
	SIZE m_sizeLogPage;

// Constructor
	CMapScrollImpl() : m_nMapMode(MM_TEXT)
	{
		::SetRectEmpty(&m_rectLogAll);
		m_sizeLogPage.cx = 0;
		m_sizeLogPage.cy = 0;
		m_sizeLogLine.cx = 0;
		m_sizeLogLine.cy = 0;
	}

// Attributes & Operations
	// mapping mode operations
	void SetScrollMapMode(int nMapMode)
	{
		ATLASSERT(nMapMode >= MM_MIN && nMapMode <= MM_MAX_FIXEDSCALE);
		m_nMapMode = nMapMode;
	}
	int GetScrollMapMode() const
	{
		ATLASSERT(m_nMapMode >= MM_MIN && m_nMapMode <= MM_MAX_FIXEDSCALE);
		return m_nMapMode;
	}

	// offset operations
	void SetScrollOffset(int x, int y, BOOL bRedraw = TRUE)
	{
		ATLASSERT(m_nMapMode >= MM_MIN && m_nMapMode <= MM_MAX_FIXEDSCALE);
		POINT ptOff = { x, y };
		// block: convert logical to device units
		{
			CWindowDC dc(NULL);
			dc.SetMapMode(m_nMapMode);
			dc.LPtoDP(&ptOff);
		}
		CScrollImpl< T >::SetScrollOffset(ptOff, bRedraw);
	}
	void SetScrollOffset(POINT ptOffset, BOOL bRedraw = TRUE)
	{
		SetScrollOffset(ptOffset.x, ptOffset.y, bRedraw);
	}
	void GetScrollOffset(POINT& ptOffset) const
	{
		ATLASSERT(m_nMapMode >= MM_MIN && m_nMapMode <= MM_MAX_FIXEDSCALE);
		ptOffset = m_ptOffset;
		// block: convert logical to device units
		{
			CWindowDC dc(NULL);
			dc.SetMapMode(m_nMapMode);
			dc.DPtoLP(&ptOffset);
		}
	}

	// size operations
	void SetScrollSize(int xMin, int yMin, int xMax, int yMax, BOOL bRedraw = TRUE)
	{
		ATLASSERT(xMax > xMin && yMax > yMin);
		ATLASSERT(m_nMapMode >= MM_MIN && m_nMapMode <= MM_MAX_FIXEDSCALE);

		::SetRect(&m_rectLogAll, xMin, yMax, xMax, yMin);

		SIZE sizeAll;
		sizeAll.cx = xMax - xMin + 1;
		sizeAll.cy = yMax - xMin + 1;
		// block: convert logical to device units
		{
			CWindowDC dc(NULL);
			dc.SetMapMode(m_nMapMode);
			dc.LPtoDP(&sizeAll);
		}
		CScrollImpl< T >::SetScrollSize(sizeAll, bRedraw);
		SetScrollLine(0, 0);
		SetScrollPage(0, 0);
	}
	void SetScrollSize(RECT& rcScroll, BOOL bRedraw = TRUE)
	{
		SetScrollSize(rcScroll.left, rcScroll.top, rcScroll.right, rcScroll.bottom, bRedraw);
	}
	void SetScrollSize(int cx, int cy, BOOL bRedraw = TRUE)
	{
		SetScrollSize(0, 0, cx, cy, bRedraw);
	}
	void SetScrollSize(SIZE size, BOOL bRedraw = NULL)
	{
		SetScrollSize(0, 0, size.cx, size.cy, bRedraw);
	}
	void GetScrollSize(RECT& rcScroll) const
	{
		ATLASSERT(m_nMapMode >= MM_MIN && m_nMapMode <= MM_MAX_FIXEDSCALE);
		rcScroll = m_rectLogAll;
	}

	// line operations
	void SetScrollLine(int cxLine, int cyLine)
	{
		ATLASSERT(cxLine >= 0 && cyLine >= 0);
		ATLASSERT(m_nMapMode >= MM_MIN && m_nMapMode <= MM_MAX_FIXEDSCALE);

		m_sizeLogLine.cx = cxLine;
		m_sizeLogLine.cy = cyLine;
		SIZE sizeLine = m_sizeLogLine;
		// block: convert logical to device units
		{
			CWindowDC dc(NULL);
			dc.SetMapMode(m_nMapMode);
			dc.LPtoDP(&sizeLine);
		}
		CScrollImpl< T >::SetScrollLine(sizeLine);
	}
	void SetScrollLine(SIZE sizeLine)
	{
		SetScrollLine(sizeLine.cx, sizeLine.cy);
	}
	void GetScrollLine(SIZE& sizeLine) const
	{
		ATLASSERT(m_nMapMode >= MM_MIN && m_nMapMode <= MM_MAX_FIXEDSCALE);
		sizeLine = m_sizeLogLine;
	}

	// page operations
	void SetScrollPage(int cxPage, int cyPage)
	{
		ATLASSERT(cxPage >= 0 && cyPage >= 0);
		ATLASSERT(m_nMapMode >= MM_MIN && m_nMapMode <= MM_MAX_FIXEDSCALE);

		m_sizeLogPage.cx = cxPage;
		m_sizeLogPage.cy = cyPage;
		SIZE sizePage = m_sizeLogPage;
		// block: convert logical to device units
		{
			CWindowDC dc(NULL);
			dc.SetMapMode(m_nMapMode);
			dc.LPtoDP(&sizePage);
		}
		CScrollImpl< T >::SetScrollPage(sizePage);
	}
	void SetScrollPage(SIZE sizePage)
	{
		SetScrollPage(sizePage.cx, sizePage.cy);
	}
	void GetScrollPage(SIZE& sizePage) const
	{
		ATLASSERT(m_nMapMode >= MM_MIN && m_nMapMode <= MM_MAX_FIXEDSCALE);
		sizePage = m_sizeLogPage;
	}

	BEGIN_MSG_MAP(CMapScrollImpl< T >)
		MESSAGE_HANDLER(WM_VSCROLL, CScrollImpl< T >::OnVScroll)
		MESSAGE_HANDLER(WM_HSCROLL, CScrollImpl< T >::OnHScroll)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, CScrollImpl< T >::OnMouseWheel)
#if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
		MESSAGE_HANDLER(m_uMsgMouseWheel, CScrollImpl< T >::OnMouseWheel)
#endif //(_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
		MESSAGE_HANDLER(WM_SETTINGCHANGE, CScrollImpl< T >::OnSettingChange)
		MESSAGE_HANDLER(WM_SIZE, CScrollImpl< T >::OnSize)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_SCROLL_UP, CScrollImpl< T >::OnScrollUp)
		COMMAND_ID_HANDLER(ID_SCROLL_DOWN, CScrollImpl< T >::OnScrollDown)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_UP, CScrollImpl< T >::OnScrollPageUp)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_DOWN, CScrollImpl< T >::OnScrollPageDown)
		COMMAND_ID_HANDLER(ID_SCROLL_TOP, CScrollImpl< T >::OnScrollTop)
		COMMAND_ID_HANDLER(ID_SCROLL_BOTTOM, CScrollImpl< T >::OnScrollBottom)
		COMMAND_ID_HANDLER(ID_SCROLL_LEFT, CScrollImpl< T >::OnScrollLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_RIGHT, CScrollImpl< T >::OnScrollRight)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_LEFT, CScrollImpl< T >::OnScrollPageLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_RIGHT, CScrollImpl< T >::OnScrollPageRight)
		COMMAND_ID_HANDLER(ID_SCROLL_ALL_LEFT, CScrollImpl< T >::OnScrollAllLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_ALL_RIGHT, CScrollImpl< T >::OnScrollAllRight)
	END_MSG_MAP()

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		ATLASSERT(::IsWindow(pT->m_hWnd));
		if(wParam != NULL)
		{
			CDCHandle dc = (HDC)wParam;
			dc.SetMapMode(m_nMapMode);
			if(m_nMapMode == MM_TEXT)
				dc.SetViewportOrg(-m_ptOffset.x, -m_ptOffset.y);
			else
				dc.SetViewportOrg(-m_ptOffset.x, -m_ptOffset.y + m_sizeAll.cy);
			dc.SetWindowOrg(m_rectLogAll.left, m_rectLogAll.bottom);
			pT->DoPaint(dc);
		}
		else
		{
			CPaintDC dc(pT->m_hWnd);
			dc.SetMapMode(m_nMapMode);
			if(m_nMapMode == MM_TEXT)
				dc.SetViewportOrg(-m_ptOffset.x, -m_ptOffset.y);
			else
				dc.SetViewportOrg(-m_ptOffset.x, -m_ptOffset.y + m_sizeAll.cy);
			dc.SetWindowOrg(m_rectLogAll.left, m_rectLogAll.bottom);
			pT->DoPaint(dc.m_hDC);
		}
		return 0;
	}
};


/////////////////////////////////////////////////////////////////////////////
// CMapScrollWindowImpl - Implements scrolling window with mapping

template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CMapScrollWindowImpl : public CWindowImpl< T, TBase, TWinTraits >, public CMapScrollImpl< T >
{
public:
	BEGIN_MSG_MAP(CMapScrollWindowImpl< T >)
		MESSAGE_HANDLER(WM_VSCROLL, CScrollImpl< T >::OnVScroll)
		MESSAGE_HANDLER(WM_HSCROLL, CScrollImpl< T >::OnHScroll)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, CScrollImpl< T >::OnMouseWheel)
#if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
		MESSAGE_HANDLER(m_uMsgMouseWheel, CScrollImpl< T >::OnMouseWheel)
#endif //(_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
		MESSAGE_HANDLER(WM_SETTINGCHANGE, CScrollImpl< T >::OnSettingChange)
		MESSAGE_HANDLER(WM_SIZE, CScrollImpl< T >::OnSize)
		MESSAGE_HANDLER(WM_PAINT, CMapScrollImpl< T >::OnPaint)
		MESSAGE_HANDLER(WM_PRINTCLIENT, CMapScrollImpl< T >::OnPaint)
	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_SCROLL_UP, CScrollImpl< T >::OnScrollUp)
		COMMAND_ID_HANDLER(ID_SCROLL_DOWN, CScrollImpl< T >::OnScrollDown)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_UP, CScrollImpl< T >::OnScrollPageUp)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_DOWN, CScrollImpl< T >::OnScrollPageDown)
		COMMAND_ID_HANDLER(ID_SCROLL_TOP, CScrollImpl< T >::OnScrollTop)
		COMMAND_ID_HANDLER(ID_SCROLL_BOTTOM, CScrollImpl< T >::OnScrollBottom)
		COMMAND_ID_HANDLER(ID_SCROLL_LEFT, CScrollImpl< T >::OnScrollLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_RIGHT, CScrollImpl< T >::OnScrollRight)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_LEFT, CScrollImpl< T >::OnScrollPageLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_PAGE_RIGHT, CScrollImpl< T >::OnScrollPageRight)
		COMMAND_ID_HANDLER(ID_SCROLL_ALL_LEFT, CScrollImpl< T >::OnScrollAllLeft)
		COMMAND_ID_HANDLER(ID_SCROLL_ALL_RIGHT, CScrollImpl< T >::OnScrollAllRight)
	END_MSG_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFSBWindow - Use as a base instead of CWindow to get flat scroll bar support

#if defined(__ATLCTRLS_H__) && (_WIN32_IE >= 0x0400)

template <class TBase = CWindow> class CFSBWindowT : public TBase, public CFlatScrollBarImpl<CFSBWindowT< TBase > >
{
public:
// Constructors
	CFSBWindowT(HWND hWnd = NULL) : TBase(hWnd)
	{ }

	CFSBWindowT< TBase >& operator=(HWND hWnd)
	{
		m_hWnd = hWnd;
		return *this;
	}

// CWindow overrides that use flat scroll bar API
// (only those methods that are used by scroll window classes)
	int SetScrollPos(int nBar, int nPos, BOOL bRedraw = TRUE)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return FlatSB_SetScrollPos(nBar, nPos, bRedraw);
	}

	BOOL GetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return FlatSB_GetScrollInfo(nBar, lpScrollInfo);
	}

	BOOL SetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return FlatSB_SetScrollInfo(nBar, lpScrollInfo, bRedraw);
	}
};

typedef CFSBWindowT<CWindow>	CFSBWindow;

#endif //defined(__ATLCTRLS_H__) && (_WIN32_IE >= 0x0400)

}; //namespace WTL

#endif //__ATLSCRL_H__
