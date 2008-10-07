// Windows Template Library - WTL version 7.0
// Copyright (C) 1997-2002 Microsoft Corporation
// All rights reserved.
//
// This file is a part of the Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.

#ifndef __ATLCTRLX_H__
#define __ATLCTRLX_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
	#error atlctrlx.h requires atlapp.h to be included first
#endif

#ifndef __ATLCTRLS_H__
	#error atlctrlx.h requires atlctrls.h to be included first
#endif


/////////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CBitmapButtonImpl<T, TBase, TWinTraits>
// CBitmapButton
// CCheckListViewCtrlImpl<T, TBase, TWinTraits>
// CCheckListViewCtrl
// CHyperLinkImpl<T, TBase, TWinTraits>
// CHyperLink
// CWaitCursor
// CMultiPaneStatusBarCtrlImpl<T, TBase>
// CMultiPaneStatusBarCtrl
// CPaneContainerImpl<T, TBase, TWinTraits>
// CPaneContainer


namespace WTL
{

/////////////////////////////////////////////////////////////////////////////
// CBitmapButton - bitmap button implementation

// bitmap button extended styles
#define BMPBTN_HOVER		0x00000001
#define BMPBTN_AUTO3D_SINGLE	0x00000002
#define BMPBTN_AUTO3D_DOUBLE	0x00000004
#define BMPBTN_AUTOSIZE		0x00000008
#define BMPBTN_SHAREIMAGELISTS	0x00000010
#define BMPBTN_AUTOFIRE		0x00000020

template <class T, class TBase = CButton, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CBitmapButtonImpl : public CWindowImpl< T, TBase, TWinTraits>
{
public:
	DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

	enum
	{
		_nImageNormal = 0,
		_nImagePushed,
		_nImageFocusOrHover,
		_nImageDisabled,

		_nImageCount = 4,
	};

	enum
	{
		ID_TIMER_FIRST = 1000,
		ID_TIMER_REPEAT = 1001
	};

	// Bitmap button specific extended styles
	DWORD m_dwExtendedStyle;

	CImageList m_ImageList;
	int m_nImage[_nImageCount];

	CToolTipCtrl m_tip;
	LPTSTR m_lpstrToolTipText;

	// Internal states
	unsigned m_fMouseOver:1;
	unsigned m_fFocus:1;
	unsigned m_fPressed:1;


// Constructor/Destructor
	CBitmapButtonImpl(DWORD dwExtendedStyle = BMPBTN_AUTOSIZE, HIMAGELIST hImageList = NULL) : 
			m_ImageList(hImageList), m_dwExtendedStyle(dwExtendedStyle), m_lpstrToolTipText(NULL),
			m_fMouseOver(0), m_fFocus(0), m_fPressed(0)
	{
		m_nImage[_nImageNormal] = -1;
		m_nImage[_nImagePushed] = -1;
		m_nImage[_nImageFocusOrHover] = -1;
		m_nImage[_nImageDisabled] = -1;
	}

	~CBitmapButtonImpl()
	{
		if((m_dwExtendedStyle & BMPBTN_SHAREIMAGELISTS) == 0)
			m_ImageList.Destroy();
		delete [] m_lpstrToolTipText;
	}

	// overridden to provide proper initialization
	BOOL SubclassWindow(HWND hWnd)
	{
		BOOL bRet = CWindowImpl< T, TBase, TWinTraits>::SubclassWindow(hWnd);
		if(bRet)
			Init();
		return bRet;
	}

// Attributes
	DWORD GetBitmapButtonExtendedStyle() const
	{
		return m_dwExtendedStyle;
	}
	DWORD SetBitmapButtonExtendedStyle(DWORD dwExtendedStyle, DWORD dwMask = 0)
	{
		DWORD dwPrevStyle = m_dwExtendedStyle;
		if(dwMask == 0)
			m_dwExtendedStyle = dwExtendedStyle;
		else
			m_dwExtendedStyle = (m_dwExtendedStyle & ~dwMask) | (dwExtendedStyle & dwMask);
		return dwPrevStyle;
	}
	HIMAGELIST GetImageList() const
	{
		return m_ImageList;
	}
	HIMAGELIST SetImageList(HIMAGELIST hImageList)
	{
		HIMAGELIST hImageListPrev = m_ImageList;
		m_ImageList = hImageList;
		if((m_dwExtendedStyle & BMPBTN_AUTOSIZE) != 0 && ::IsWindow(m_hWnd))
			SizeToImage();
		return hImageListPrev;
	}
	int GetToolTipTextLength() const
	{
		return (m_lpstrToolTipText == NULL) ? -1 : lstrlen(m_lpstrToolTipText);
	}
	bool GetToolTipText(LPTSTR lpstrText, int nLength) const
	{
		ATLASSERT(lpstrText != NULL);
		if(m_lpstrToolTipText == NULL)
			return false;
		return (lstrcpyn(lpstrText, m_lpstrToolTipText, min(nLength, lstrlen(m_lpstrToolTipText) + 1)) != NULL);
	}
	bool SetToolTipText(LPCTSTR lpstrText)
	{
		if(m_lpstrToolTipText != NULL)
		{
			delete [] m_lpstrToolTipText;
			m_lpstrToolTipText = NULL;
		}
		if(lpstrText == NULL)
		{
			if(m_tip.IsWindow())
				m_tip.Activate(FALSE);
			return true;
		}
		ATLTRY(m_lpstrToolTipText = new TCHAR[lstrlen(lpstrText) + 1]);
		if(m_lpstrToolTipText == NULL)
			return false;
		bool bRet = (lstrcpy(m_lpstrToolTipText, lpstrText) != NULL);
		if(bRet && m_tip.IsWindow())
		{
			m_tip.Activate(TRUE);
			m_tip.AddTool(m_hWnd, m_lpstrToolTipText);
		}
		return bRet;
	}

// Operations
	void SetImages(int nNormal, int nPushed = -1, int nFocusOrHover = -1, int nDisabled = -1)
	{
		if(nNormal != -1)
			m_nImage[_nImageNormal] = nNormal;
		if(nPushed != -1)
			m_nImage[_nImagePushed] = nPushed;
		if(nFocusOrHover != -1)
			m_nImage[_nImageFocusOrHover] = nFocusOrHover;
		if(nDisabled != -1)
			m_nImage[_nImageDisabled] = nDisabled;
	}
	BOOL SizeToImage()
	{
		ATLASSERT(::IsWindow(m_hWnd) && m_ImageList.m_hImageList != NULL);
		int cx = 0;
		int cy = 0;
		if(!m_ImageList.GetIconSize(cx, cy))
			return FALSE;
		return ResizeClient(cx, cy);
	}

// Overrideables
	void DoPaint(CDCHandle dc)
	{
		ATLASSERT(m_ImageList.m_hImageList != NULL);	// image list must be set
		ATLASSERT(m_nImage[0] != -1);			// main bitmap must be set

		// set bitmap according to the current button state
		int nImage = -1;
		bool bHover = IsHoverMode();
		if(m_fPressed == 1)
			nImage = m_nImage[_nImagePushed];
		else if((!bHover && m_fFocus == 1) || (bHover && m_fMouseOver == 1))
			nImage = m_nImage[_nImageFocusOrHover];
		else if(!IsWindowEnabled())
			nImage = m_nImage[_nImageDisabled];
		if(nImage == -1)	// not there, use default one
			nImage = m_nImage[_nImageNormal];

		// draw the button image
		int xyPos = 0;
		if((m_fPressed == 1) && ((m_dwExtendedStyle & (BMPBTN_AUTO3D_SINGLE | BMPBTN_AUTO3D_DOUBLE)) != 0) && (m_nImage[_nImagePushed] == -1))
			xyPos = 1;
		m_ImageList.Draw(dc, nImage, xyPos, xyPos, ILD_NORMAL);

		// draw 3D border if required
		if((m_dwExtendedStyle & (BMPBTN_AUTO3D_SINGLE | BMPBTN_AUTO3D_DOUBLE)) != 0)
		{
			RECT rect;
			GetClientRect(&rect);

			if(m_fPressed == 1)
				dc.DrawEdge(&rect, ((m_dwExtendedStyle & BMPBTN_AUTO3D_SINGLE) != 0) ? BDR_SUNKENOUTER : EDGE_SUNKEN, BF_RECT);
			else if(!bHover || m_fMouseOver == 1)
				dc.DrawEdge(&rect, ((m_dwExtendedStyle & BMPBTN_AUTO3D_SINGLE) != 0) ? BDR_RAISEDINNER : EDGE_RAISED, BF_RECT);

			if(!bHover && m_fFocus == 1)
			{
				::InflateRect(&rect, -2 * ::GetSystemMetrics(SM_CXEDGE), -2 * ::GetSystemMetrics(SM_CYEDGE));
				dc.DrawFocusRect(&rect);
			}
		}
	}

// Message map and handlers
	typedef CBitmapButtonImpl< T, TBase, TWinTraits >	thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseMessage)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
		MESSAGE_HANDLER(WM_ENABLE, OnEnable)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		Init();
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		MSG msg = { m_hWnd, uMsg, wParam, lParam };
		if(m_tip.IsWindow())
			m_tip.RelayEvent(&msg);
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return 1;	// no background needed
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		if(wParam != NULL)
		{
			pT->DoPaint((HDC)wParam);
		}
		else
		{
			CPaintDC dc(m_hWnd);
			pT->DoPaint(dc.m_hDC);
		}
		return 0;
	}

	LRESULT OnFocus(UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		m_fFocus = (uMsg == WM_SETFOCUS) ? 1 : 0;
		Invalidate();
		UpdateWindow();
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = 0;
		if(IsHoverMode())
			SetCapture();
		else
			lRet = DefWindowProc(uMsg, wParam, lParam);
		if(::GetCapture() == m_hWnd)
		{
			m_fPressed = 1;
			Invalidate();
			UpdateWindow();
		}
		if((m_dwExtendedStyle & BMPBTN_AUTOFIRE) != 0)
		{
			int nElapse = 250;
			int nDelay = 0;
			if(::SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &nDelay, 0))
				nElapse += nDelay * 250;	// all milli-seconds
			SetTimer(ID_TIMER_FIRST, nElapse);
		}
		return lRet;
	}

	LRESULT OnLButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = 0;
		if(!IsHoverMode())
			lRet = DefWindowProc(uMsg, wParam, lParam);
		if(::GetCapture() != m_hWnd)
			SetCapture();
		if(m_fPressed == 0)
		{
			m_fPressed = 1;
			Invalidate();
			UpdateWindow();
		}
		return lRet;
	}

	LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = 0;
		bool bHover = IsHoverMode();
		if(!bHover)
			lRet = DefWindowProc(uMsg, wParam, lParam);
		if(::GetCapture() == m_hWnd)
		{
			if(bHover && m_fPressed == 1)
				::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
			::ReleaseCapture();
		}
		return lRet;
	}

	LRESULT OnCaptureChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(m_fPressed == 1)
		{
			m_fPressed = 0;
			Invalidate();
			UpdateWindow();
		}
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnEnable(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		Invalidate();
		UpdateWindow();
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		if(::GetCapture() == m_hWnd)
		{
			POINT ptCursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ClientToScreen(&ptCursor);
			RECT rect;
			GetWindowRect(&rect);
			unsigned int uPressed = ::PtInRect(&rect, ptCursor) ? 1 : 0;
			if(m_fPressed != uPressed)
			{
				m_fPressed = uPressed;
				Invalidate();
				UpdateWindow();
			}
		}
		else if(IsHoverMode() && m_fMouseOver == 0)
		{
			m_fMouseOver = 1;
			Invalidate();
			UpdateWindow();
			StartTrackMouseLeave();
		}
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		if(m_fMouseOver == 1)
		{
			m_fMouseOver = 0;
			Invalidate();
			UpdateWindow();
		}
		return 0;
	}

	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(wParam == VK_SPACE && IsHoverMode())
			return 0;	// ignore if in hover mode
		if(wParam == VK_SPACE && m_fPressed == 0)
		{
			m_fPressed = 1;
			Invalidate();
			UpdateWindow();
		}
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(wParam == VK_SPACE && IsHoverMode())
			return 0;	// ignore if in hover mode
		if(wParam == VK_SPACE && m_fPressed == 1)
		{
			m_fPressed = 0;
			Invalidate();
			UpdateWindow();
		}
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ATLASSERT((m_dwExtendedStyle & BMPBTN_AUTOFIRE) != 0);
		switch(wParam)	// timer ID
		{
		case ID_TIMER_FIRST:
			KillTimer(ID_TIMER_FIRST);
			if(m_fPressed == 1)
			{
				::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
				int nElapse = 250;
				int nRepeat = 40;
				if(::SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &nRepeat, 0))
					nElapse = 10000 / (10 * nRepeat + 25);	// milli-seconds, approximated
				SetTimer(ID_TIMER_REPEAT, nElapse);
			}
			break;
		case ID_TIMER_REPEAT:
			if(m_fPressed == 1)
				::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
			else if(::GetCapture() != m_hWnd)
				KillTimer(ID_TIMER_REPEAT);
			break;
		default:	// not our timer
			break;
		}
		return 0;
	}

// Implementation
	void Init()
	{
		// We need this style to prevent Windows from painting the button
		ModifyStyle(0, BS_OWNERDRAW);

		// create a tool tip
		m_tip.Create(m_hWnd);
		ATLASSERT(m_tip.IsWindow());
		if(m_tip.IsWindow() && m_lpstrToolTipText != NULL)
		{
			m_tip.Activate(TRUE);
			m_tip.AddTool(m_hWnd, m_lpstrToolTipText);
		}

		if(m_ImageList.m_hImageList != NULL && (m_dwExtendedStyle & BMPBTN_AUTOSIZE) != 0)
			SizeToImage();
	}

	BOOL StartTrackMouseLeave()
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		return _TrackMouseEvent(&tme);
	}

	bool IsHoverMode() const
	{
		return ((m_dwExtendedStyle & BMPBTN_HOVER) != 0);
	}
};


class CBitmapButton : public CBitmapButtonImpl<CBitmapButton>
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_BitmapButton"), GetWndClassName())

	CBitmapButton(DWORD dwExtendedStyle = BMPBTN_AUTOSIZE, HIMAGELIST hImageList = NULL) : 
		CBitmapButtonImpl<CBitmapButton>(dwExtendedStyle, hImageList)
	{ }
};


/////////////////////////////////////////////////////////////////////////////
// CCheckListCtrlView - list view control with check boxes

template <DWORD t_dwStyle, DWORD t_dwExStyle, DWORD t_dwExListViewStyle>
class CCheckListViewCtrlImplTraits
{
public:
	static DWORD GetWndStyle(DWORD dwStyle)
	{
		return (dwStyle == 0) ? t_dwStyle : dwStyle;
	}
	static DWORD GetWndExStyle(DWORD dwExStyle)
	{
		return (dwExStyle == 0) ? t_dwExStyle : dwExStyle;
	}
	static DWORD GetExtendedLVStyle()
	{
		return t_dwExListViewStyle;
	}
};

typedef CCheckListViewCtrlImplTraits<WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT>	CCheckListViewCtrlTraits;

template <class T, class TBase = CListViewCtrl, class TWinTraits = CCheckListViewCtrlTraits>
class ATL_NO_VTABLE CCheckListViewCtrlImpl : public CWindowImpl<T, TBase, TWinTraits>
{
public:
	DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

// Attributes
	static DWORD GetExtendedLVStyle()
	{
		return TWinTraits::GetExtendedLVStyle();
	}

// Operations
	BOOL SubclassWindow(HWND hWnd)
	{
		BOOL bRet = CWindowImplBaseT< TBase, TWinTraits>::SubclassWindow(hWnd);
		if(bRet)
		{
			T* pT = static_cast<T*>(this);
			pT;
			ATLASSERT((pT->GetExtendedLVStyle() & LVS_EX_CHECKBOXES) != 0);
			SetExtendedListViewStyle(pT->GetExtendedLVStyle());
		}
		return bRet;
	}

	void CheckSelectedItems(int nCurrItem)
	{
		// first check if this item is selected
		LVITEM lvi;
		lvi.iItem = nCurrItem;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_STATE;
		lvi.stateMask = LVIS_SELECTED;
		GetItem(&lvi);
		// if item is not selected, don't do anything
		if(!(lvi.state & LVIS_SELECTED))
			return;
		// new check state will be reverse of the current state,
		BOOL bCheck = !GetCheckState(nCurrItem);
		int nItem = -1;
		int nOldItem = -1;
		while((nItem = GetNextItem(nOldItem, LVNI_SELECTED)) != -1)
		{
			if(nItem != nCurrItem)
				SetCheckState(nItem, bCheck);
			nOldItem = nItem;
		}
	}

// Implementation
	typedef CCheckListViewCtrlImpl< T, TBase, TWinTraits >	thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDown)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		// first let list view control initialize everything
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);
		T* pT = static_cast<T*>(this);
		pT;
		ATLASSERT((pT->GetExtendedLVStyle() & LVS_EX_CHECKBOXES) != 0);
		SetExtendedListViewStyle(pT->GetExtendedLVStyle());
		return lRet;
	}

	LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		POINT ptMsg = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		LVHITTESTINFO lvh;
		lvh.pt = ptMsg;
		if(HitTest(&lvh) != -1 && lvh.flags == LVHT_ONITEMSTATEICON && ::GetKeyState(VK_CONTROL) >= 0)
			CheckSelectedItems(lvh.iItem);
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(wParam == VK_SPACE)
		{
			int nCurrItem = GetNextItem(-1, LVNI_FOCUSED);
			if(nCurrItem != -1  && ::GetKeyState(VK_CONTROL) >= 0)
				CheckSelectedItems(nCurrItem);
		}
		bHandled = FALSE;
		return 1;
	}
};

class CCheckListViewCtrl : public CCheckListViewCtrlImpl<CCheckListViewCtrl>
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_CheckListView"), GetWndClassName())
};


/////////////////////////////////////////////////////////////////////////////
// CHyperLink - hyper link control implementation

#if (WINVER < 0x0500)
__declspec(selectany) struct
{
	enum { cxWidth = 32, cyHeight = 32 };
	int xHotSpot;
	int yHotSpot;
	unsigned char arrANDPlane[cxWidth * cyHeight / 8];
	unsigned char arrXORPlane[cxWidth * cyHeight / 8];
} _AtlHyperLink_CursorData = 
{
	5, 0, 
	{
		0xF9, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 
		0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0x3F, 0xFF, 0xFF, 0xF0, 0x07, 0xFF, 0xFF, 0xF0, 0x01, 0xFF, 0xFF, 
		0xF0, 0x00, 0xFF, 0xFF, 0x10, 0x00, 0x7F, 0xFF, 0x00, 0x00, 0x7F, 0xFF, 0x00, 0x00, 0x7F, 0xFF, 
		0x80, 0x00, 0x7F, 0xFF, 0xC0, 0x00, 0x7F, 0xFF, 0xC0, 0x00, 0x7F, 0xFF, 0xE0, 0x00, 0x7F, 0xFF, 
		0xE0, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xF8, 0x01, 0xFF, 0xFF, 
		0xF8, 0x01, 0xFF, 0xFF, 0xF8, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 
		0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0xC0, 0x00, 0x00, 0x06, 0xD8, 0x00, 0x00, 
		0x06, 0xDA, 0x00, 0x00, 0x06, 0xDB, 0x00, 0x00, 0x67, 0xFB, 0x00, 0x00, 0x77, 0xFF, 0x00, 0x00, 
		0x37, 0xFF, 0x00, 0x00, 0x17, 0xFF, 0x00, 0x00, 0x1F, 0xFF, 0x00, 0x00, 0x0F, 0xFF, 0x00, 0x00, 
		0x0F, 0xFE, 0x00, 0x00, 0x07, 0xFE, 0x00, 0x00, 0x07, 0xFE, 0x00, 0x00, 0x03, 0xFC, 0x00, 0x00, 
		0x03, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	}
};
#endif //(WINVER < 0x0500)


template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CHyperLinkImpl : public CWindowImpl< T, TBase, TWinTraits >
{
public:
	LPTSTR m_lpstrLabel;
	LPTSTR m_lpstrHyperLink;
	HCURSOR m_hCursor;
	HFONT m_hFont;
	RECT m_rcLink;
	bool m_bPaintLabel;
	CToolTipCtrl m_tip;

	bool m_bVisited;
	COLORREF m_clrLink;
	COLORREF m_clrVisited;


// Constructor/Destructor
	CHyperLinkImpl() : m_lpstrLabel(NULL), m_lpstrHyperLink(NULL),
			m_hCursor(NULL), m_hFont(NULL), m_bPaintLabel(true), m_bVisited(false),
			m_clrLink(RGB(0, 0, 255)), m_clrVisited(RGB(128, 0, 128))
	{
		::SetRectEmpty(&m_rcLink);
	}

	~CHyperLinkImpl()
	{
		free(m_lpstrLabel);
		free(m_lpstrHyperLink);
		if(m_hFont != NULL)
			::DeleteObject(m_hFont);
#if (WINVER < 0x0500)
		// It was created, not loaded, so we have to destroy it
		if(m_hCursor != NULL)
			::DestroyCursor(m_hCursor);
#endif //(WINVER < 0x0500)
	}

// Attributes
	bool GetLabel(LPTSTR lpstrBuffer, int nLength) const
	{
		if(m_lpstrLabel == NULL)
			return false;
		ATLASSERT(lpstrBuffer != NULL);
		if(nLength > lstrlen(m_lpstrLabel) + 1)
		{
			lstrcpy(lpstrBuffer, m_lpstrLabel);
			return true;
		}
		return false;
	}

	bool SetLabel(LPCTSTR lpstrLabel)
	{
		free(m_lpstrLabel);
		m_lpstrLabel = NULL;
		ATLTRY(m_lpstrLabel = (LPTSTR)malloc((lstrlen(lpstrLabel) + 1) * sizeof(TCHAR)));
		if(m_lpstrLabel == NULL)
			return false;
		lstrcpy(m_lpstrLabel, lpstrLabel);
		T* pT = static_cast<T*>(this);
		pT->CalcLabelRect();
		return true;
	}

	bool GetHyperLink(LPTSTR lpstrBuffer, int nLength) const
	{
		if(m_lpstrHyperLink == NULL)
			return false;
		ATLASSERT(lpstrBuffer != NULL);
		if(nLength > lstrlen(m_lpstrHyperLink) + 1)
		{
			lstrcpy(lpstrBuffer, m_lpstrHyperLink);
			return true;
		}
		return false;
	}

	bool SetHyperLink(LPCTSTR lpstrLink)
	{
		free(m_lpstrHyperLink);
		m_lpstrHyperLink = NULL;
		ATLTRY(m_lpstrHyperLink = (LPTSTR)malloc((lstrlen(lpstrLink) + 1) * sizeof(TCHAR)));
		if(m_lpstrHyperLink == NULL)
			return false;
		lstrcpy(m_lpstrHyperLink, lpstrLink);
		if(m_lpstrLabel == NULL)
		{
			T* pT = static_cast<T*>(this);
			pT->CalcLabelRect();
		}
		return true;
	}

// Operations
	BOOL SubclassWindow(HWND hWnd)
	{
		ATLASSERT(m_hWnd == NULL);
		ATLASSERT(::IsWindow(hWnd));
		BOOL bRet = CWindowImpl< T, TBase, TWinTraits >::SubclassWindow(hWnd);
		if(bRet)
			Init();
		return bRet;
	}

	bool Navigate()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_lpstrHyperLink != NULL);
		DWORD_PTR dwRet = (DWORD_PTR)::ShellExecute(0, _T("open"), m_lpstrHyperLink, 0, 0, SW_SHOWNORMAL);
		ATLASSERT(dwRet > 32);
		if(dwRet > 32)
		{
			m_bVisited = true;
			Invalidate();
		}
		return (dwRet > 32);
	}

// Message map and handlers
	BEGIN_MSG_MAP(CHyperLinkImpl)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouseMessage)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
		MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		Init();
		return 0;
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(m_bPaintLabel)
		{
			HBRUSH hBrush = (HBRUSH)::SendMessage(GetParent(), WM_CTLCOLORSTATIC, wParam, (LPARAM)m_hWnd);
			if(hBrush != NULL)
			{
				CDCHandle dc = (HDC)wParam;
				RECT rect;
				GetClientRect(&rect);
				dc.FillRect(&rect, hBrush);
			}
		}
		else
		{
			bHandled = FALSE;
		}
		return 1;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(!m_bPaintLabel)
		{
			bHandled = FALSE;
			return 1;
		}

		T* pT = static_cast<T*>(this);
		if(wParam != NULL)
		{
			pT->DoPaint((HDC)wParam);
		}
		else
		{
			CPaintDC dc(m_hWnd);
			pT->DoPaint(dc.m_hDC);
		}

		return 0;
	}

	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(m_bPaintLabel)
			Invalidate();
		else
			bHandled = FALSE;
		return 0;
	}

	LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		if(m_lpstrHyperLink != NULL && ::PtInRect(&m_rcLink, pt))
			::SetCursor(m_hCursor);
		else
			bHandled = FALSE;
		return 0;
	}

	LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		if(::PtInRect(&m_rcLink, pt))
		{
			SetFocus();
			SetCapture();
		}
		return 0;
	}

	LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		if(GetCapture() == m_hWnd)
		{
			ReleaseCapture();
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			if(::PtInRect(&m_rcLink, pt))
			{
				T* pT = static_cast<T*>(this);
				pT->Navigate();
			}
		}
		return 0;
	}

	LRESULT OnSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);
		if(m_lpstrHyperLink != NULL && ::PtInRect(&m_rcLink, pt))
		{
			return TRUE;
		}
		bHandled = FALSE;
		return FALSE;
	}

	LRESULT OnChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		if(wParam == VK_RETURN || wParam == VK_SPACE)
		{
			T* pT = static_cast<T*>(this);
			pT->Navigate();
		}
		return 0;
	}

	LRESULT OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return DLGC_WANTCHARS;
	}

	LRESULT OnMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		MSG msg = { m_hWnd, uMsg, wParam, lParam };
		if(m_tip.IsWindow())
			m_tip.RelayEvent(&msg);
		bHandled = FALSE;
		return 1;
	}

// Implementation
	void Init()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		// Check if we should paint a label
		TCHAR lpszBuffer[8];
		if(::GetClassName(m_hWnd, lpszBuffer, 8))
		{
			if(lstrcmpi(lpszBuffer, _T("static")) == 0)
			{
				ModifyStyle(0, SS_NOTIFY);	// we need this
				DWORD dwStyle = GetStyle() & 0x000000FF;
				if(dwStyle == SS_ICON || dwStyle == SS_BLACKRECT || dwStyle == SS_GRAYRECT || 
						dwStyle == SS_WHITERECT || dwStyle == SS_BLACKFRAME || dwStyle == SS_GRAYFRAME || 
						dwStyle == SS_WHITEFRAME || dwStyle == SS_OWNERDRAW || 
						dwStyle == SS_BITMAP || dwStyle == SS_ENHMETAFILE)
					m_bPaintLabel = false;
			}
		}

		// create or load a cursor
#if (WINVER >= 0x0500)
		m_hCursor = ::LoadCursor(NULL, IDC_HAND);
#else
		m_hCursor = ::CreateCursor(_Module.GetModuleInstance(), _AtlHyperLink_CursorData.xHotSpot, _AtlHyperLink_CursorData.yHotSpot, _AtlHyperLink_CursorData.cxWidth, _AtlHyperLink_CursorData.cyHeight, _AtlHyperLink_CursorData.arrANDPlane, _AtlHyperLink_CursorData.arrXORPlane);
#endif //!(WINVER >= 0x0500)
		ATLASSERT(m_hCursor != NULL);

		// set font
		if(m_bPaintLabel)
		{
			CWindow wnd = GetParent();
			CFontHandle font = wnd.GetFont();
			if(font.m_hFont != NULL)
			{
				LOGFONT lf;
				font.GetLogFont(&lf);
				lf.lfUnderline = TRUE;
				m_hFont = ::CreateFontIndirect(&lf);
			}
		}

		// set label (defaults to window text)
		if(m_lpstrLabel == NULL)
		{
			int nLen = GetWindowTextLength();
			if(nLen > 0)
			{
				LPTSTR lpszText = (LPTSTR)_alloca((nLen+1)*sizeof(TCHAR));
				if(GetWindowText(lpszText, nLen+1))
					SetLabel(lpszText);
			}
		}

		// set hyperlink (defaults to label)
		if(m_lpstrHyperLink == NULL && m_lpstrLabel != NULL)
			SetHyperLink(m_lpstrLabel);

		T* pT = static_cast<T*>(this);
		pT->CalcLabelRect();

		// create a tool tip
		m_tip.Create(m_hWnd);
		ATLASSERT(m_tip.IsWindow());
		m_tip.Activate(TRUE);
		m_tip.AddTool(m_hWnd, m_lpstrHyperLink);

		// set link colors
		if(m_bPaintLabel)
		{
			CRegKey rk;
			LONG lRet = rk.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Internet Explorer\\Settings"));
			if(lRet == 0)
			{
				TCHAR szBuff[12];
#if (_ATL_VER >= 0x0700)
				ULONG ulCount = 12 * sizeof(TCHAR);
				lRet = rk.QueryStringValue(_T("Anchor Color"), szBuff, &ulCount);
#else
				DWORD dwCount = 12 * sizeof(TCHAR);
				lRet = rk.QueryValue(szBuff, _T("Anchor Color"), &dwCount);
#endif
				if(lRet == 0)
				{
					COLORREF clr = _ParseColorString(szBuff);
					ATLASSERT(clr != CLR_INVALID);
					if(clr != CLR_INVALID)
						m_clrLink = clr;
				}

#if (_ATL_VER >= 0x0700)
				ulCount = 12 * sizeof(TCHAR);
				lRet = rk.QueryStringValue(_T("Anchor Color Visited"), szBuff, &ulCount);
#else
				dwCount = 12 * sizeof(TCHAR);
				lRet = rk.QueryValue(szBuff, _T("Anchor Color Visited"), &dwCount);
#endif
				if(lRet == 0)
				{
					COLORREF clr = _ParseColorString(szBuff);
					ATLASSERT(clr != CLR_INVALID);
					if(clr != CLR_INVALID)
						m_clrVisited = clr;
				}
			}
		}
	}

	static COLORREF _ParseColorString(LPTSTR lpstr)
	{
		int c[3] = { -1, -1, -1 };
		LPTSTR p;
		for(int i = 0; i < 2; i++)
		{
			for(p = lpstr; *p != _T('\0'); p = ::CharNext(p))
			{
				if(*p == _T(','))
				{
					*p = _T('\0');
					c[i] = _ttoi(lpstr);
					lpstr = &p[1];
					break;
				}
			}
			if(c[i] == -1)
				return CLR_INVALID;
		}
		if(*lpstr == _T('\0'))
			return CLR_INVALID;
		c[2] = _ttoi(lpstr);

		return RGB(c[0], c[1], c[2]);
	}

	bool CalcLabelRect()
	{
		if(!::IsWindow(m_hWnd))
			return false;
		if(m_lpstrLabel == NULL && m_lpstrHyperLink == NULL)
			return false;

		CClientDC dc(m_hWnd);
		RECT rect;
		GetClientRect(&rect);
		m_rcLink = rect;
		if(m_bPaintLabel)
		{
			HFONT hOldFont = NULL;
			if(m_hFont != NULL)
				hOldFont = dc.SelectFont(m_hFont);
			LPTSTR lpstrText = (m_lpstrLabel != NULL) ? m_lpstrLabel : m_lpstrHyperLink;
			DWORD dwStyle = GetStyle();
			int nDrawStyle = DT_LEFT;
			if (dwStyle & SS_CENTER)
				nDrawStyle = DT_CENTER;
			else if (dwStyle & SS_RIGHT)
				nDrawStyle = DT_RIGHT;
			dc.DrawText(lpstrText, -1, &m_rcLink, nDrawStyle | DT_WORDBREAK | DT_CALCRECT);
			if(m_hFont != NULL)
				dc.SelectFont(hOldFont);
			if (dwStyle & SS_CENTER)
			{
				int dx = (rect.right - m_rcLink.right) / 2;
				::OffsetRect(&m_rcLink, dx, 0);
			}
			else if (dwStyle & SS_RIGHT)
			{
				int dx = rect.right - m_rcLink.right;
				::OffsetRect(&m_rcLink, dx, 0);
			}
		}

		return true;
	}

	void DoPaint(CDCHandle dc)
	{
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(m_bVisited ? m_clrVisited : m_clrLink);
		if(m_hFont != NULL)
			dc.SelectFont(m_hFont);
		LPTSTR lpstrText = (m_lpstrLabel != NULL) ? m_lpstrLabel : m_lpstrHyperLink;
		DWORD dwStyle = GetStyle();
		int nDrawStyle = DT_LEFT;
		if (dwStyle & SS_CENTER)
			nDrawStyle = DT_CENTER;
		else if (dwStyle & SS_RIGHT)
			nDrawStyle = DT_RIGHT;
		dc.DrawText(lpstrText, -1, &m_rcLink, nDrawStyle | DT_WORDBREAK);
		if(GetFocus() == m_hWnd)
			dc.DrawFocusRect(&m_rcLink);
	}
};


class CHyperLink : public CHyperLinkImpl<CHyperLink>
{
public:
	DECLARE_WND_CLASS(_T("WTL_HyperLink"))
};


/////////////////////////////////////////////////////////////////////////////
// CWaitCursor - displays a wait cursor

class CWaitCursor
{
public:
// Data
	HCURSOR m_hWaitCursor;
	HCURSOR m_hOldCursor;
	bool m_bInUse;

// Constructor/destructor
	CWaitCursor(bool bSet = true, LPCTSTR lpstrCursor = IDC_WAIT, bool bSys = true) : m_hOldCursor(NULL), m_bInUse(false)
	{
		HINSTANCE hInstance = bSys ? NULL : _Module.GetResourceInstance();
		m_hWaitCursor = ::LoadCursor(hInstance, lpstrCursor);
		ATLASSERT(m_hWaitCursor != NULL);

		if(bSet)
			Set();
	}

	~CWaitCursor()
	{
		Restore();
	}

// Methods
	bool Set()
	{
		if(m_bInUse)
			return false;
		m_hOldCursor = ::SetCursor(m_hWaitCursor);
		m_bInUse = true;
		return true;
	}

	bool Restore()
	{
		if(!m_bInUse)
			return false;
		::SetCursor(m_hOldCursor);
		m_bInUse = false;
		return true;
	}
};


/////////////////////////////////////////////////////////////////////////////
// CMultiPaneStatusBarCtrl - Status Bar with multiple panes

template <class T, class TBase = CStatusBarCtrl>
class ATL_NO_VTABLE CMultiPaneStatusBarCtrlImpl : public CWindowImpl< T, TBase >
{
public:
	DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

// Data
	enum { m_cxPaneMargin = 3 };

	int m_nPanes;
	int* m_pPane;

// Constructor/destructor
	CMultiPaneStatusBarCtrlImpl() : m_nPanes(0), m_pPane(NULL)
	{ }

	~CMultiPaneStatusBarCtrlImpl()
	{
		delete [] m_pPane;
	}

// Methods
	HWND Create(HWND hWndParent, LPCTSTR lpstrText, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP, UINT nID = ATL_IDW_STATUS_BAR)
	{
		return CWindowImpl< T, TBase >::Create(hWndParent, rcDefault, lpstrText, dwStyle, 0, nID);
	}

	HWND Create(HWND hWndParent, UINT nTextID = ATL_IDS_IDLEMESSAGE, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP, UINT nID = ATL_IDW_STATUS_BAR)
	{
		TCHAR szText[128];	// max text lentgth is 127 for status bars
		szText[0] = 0;
		::LoadString(_Module.GetResourceInstance(), nTextID, szText, 127);
		return Create(hWndParent, szText, dwStyle, nID);
	}

	BOOL SetPanes(int* pPanes, int nPanes, bool bSetText = true)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(nPanes > 0);

		m_nPanes = nPanes;
		delete [] m_pPane;
		m_pPane = NULL;

		ATLTRY(m_pPane = new int[nPanes]);
		ATLASSERT(m_pPane != NULL);
		if(m_pPane == NULL)
			return FALSE;
		memcpy(m_pPane, pPanes, nPanes * sizeof(int));

		int* pPanesPos = NULL;
		ATLTRY(pPanesPos = (int*)_alloca(nPanes * sizeof(int)));
		ATLASSERT(pPanesPos != NULL);

		// get status bar DC and set font
		CClientDC dc(m_hWnd);
		HFONT hOldFont = dc.SelectFont(GetFont());

		// get status bar borders
		int arrBorders[3];
		GetBorders(arrBorders);

		TCHAR szBuff[256];
		SIZE size;
		int cxLeft = arrBorders[0];

		// calculate right edge of each part
		for(int i = 0; i < nPanes; i++)
		{
			if(pPanes[i] == ID_DEFAULT_PANE)
			{
				// will be resized later
				pPanesPos[i] = 100 + cxLeft + arrBorders[2];
			}
			else
			{
				::LoadString(_Module.GetResourceInstance(), pPanes[i], szBuff, sizeof(szBuff) / sizeof(TCHAR));
				dc.GetTextExtent(szBuff, lstrlen(szBuff), &size);
				T* pT = static_cast<T*>(this);
				pT;
				pPanesPos[i] = cxLeft + size.cx + arrBorders[2] + 2 * pT->m_cxPaneMargin;
			}
			cxLeft = pPanesPos[i];
		}

		BOOL bRet = SetParts(nPanes, pPanesPos);

		if(bRet && bSetText)
		{
			for(int i = 0; i < nPanes; i++)
			{
				if(pPanes[i] != ID_DEFAULT_PANE)
				{
					::LoadString(_Module.GetResourceInstance(), pPanes[i], szBuff, sizeof(szBuff) / sizeof(TCHAR));
					SetPaneText(m_pPane[i], szBuff);
				}
			}
		}

		dc.SelectFont(hOldFont);
		return bRet;
	}

	bool GetPaneTextLength(int nPaneID, int* pcchLength = NULL, int* pnType = NULL) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int nIndex  = GetPaneIndexFromID(nPaneID);
		if(nIndex == -1)
			return false;

		int nLength = GetTextLength(nIndex, pnType);
		if(pcchLength != NULL)
			*pcchLength = nLength;

		return true;
	}

	BOOL GetPaneText(int nPaneID, LPTSTR lpstrText, int* pcchLength = NULL, int* pnType = NULL) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int nIndex  = GetPaneIndexFromID(nPaneID);
		if(nIndex == -1)
			return FALSE;

		int nLength = GetText(nIndex, lpstrText, pnType);
		if(pcchLength != NULL)
			*pcchLength = nLength;

		return TRUE;
	}

	BOOL SetPaneText(int nPaneID, LPCTSTR lpstrText, int nType = 0)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int nIndex  = GetPaneIndexFromID(nPaneID);
		if(nIndex == -1)
			return FALSE;

		return SetText(nIndex, lpstrText, nType);
	}

	BOOL GetPaneRect(int nPaneID, LPRECT lpRect) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int nIndex  = GetPaneIndexFromID(nPaneID);
		if(nIndex == -1)
			return FALSE;

		return GetRect(nIndex, lpRect);
	}

	BOOL SetPaneWidth(int nPaneID, int cxWidth)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(nPaneID != ID_DEFAULT_PANE);	// Can't resize this one
		int nIndex  = GetPaneIndexFromID(nPaneID);
		if(nIndex == -1)
			return FALSE;

		// get pane positions
		int* pPanesPos = NULL;
		ATLTRY(pPanesPos = (int*)_alloca(m_nPanes * sizeof(int)));
		GetParts(m_nPanes, pPanesPos);
		// calculate offset
		int cxPaneWidth = pPanesPos[nIndex] - ((nIndex == 0) ? 0 : pPanesPos[nIndex - 1]);
		int cxOff = cxWidth - cxPaneWidth;
		// find variable width pane
		int nDef = m_nPanes;
		for(int i = 0; i < m_nPanes; i++)
		{
			if(m_pPane[i] == ID_DEFAULT_PANE)
			{
				nDef = i;
				break;
			}
		}
		// resize
		if(nIndex < nDef)	// before default pane
		{
			for(int i = nIndex; i < nDef; i++)
				pPanesPos[i] += cxOff;
				
		}
		else			// after default one
		{
			for(int i = nDef; i < nIndex; i++)
				pPanesPos[i] -= cxOff;
		}
		// set pane postions
		return SetParts(m_nPanes, pPanesPos);
	}

#if (_WIN32_IE >= 0x0400)
	BOOL GetPaneTipText(int nPaneID, LPTSTR lpstrText, int nSize) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int nIndex  = GetPaneIndexFromID(nPaneID);
		if(nIndex == -1)
			return FALSE;

		GetTipText(nIndex, lpstrText, nSize);
		return TRUE;
	}

	BOOL SetPaneTipText(int nPaneID, LPCTSTR lpstrText)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int nIndex  = GetPaneIndexFromID(nPaneID);
		if(nIndex == -1)
			return FALSE;

		SetTipText(nIndex, lpstrText);
		return TRUE;
	}

	BOOL GetPaneIcon(int nPaneID, HICON& hIcon) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int nIndex  = GetPaneIndexFromID(nPaneID);
		if(nIndex == -1)
			return FALSE;

		hIcon = GetIcon(nIndex);
		return TRUE;
	}

	BOOL SetPaneIcon(int nPaneID, HICON hIcon)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int nIndex  = GetPaneIndexFromID(nPaneID);
		if(nIndex == -1)
			return FALSE;

		return SetIcon(nIndex, hIcon);
	}
#endif //(_WIN32_IE >= 0x0400)

// Message map and handlers
	BEGIN_MSG_MAP(CMultiPaneStatusBarCtrlImpl< T >)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
	END_MSG_MAP()

	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);
		if(wParam != SIZE_MINIMIZED && m_nPanes > 0)
		{
			T* pT = static_cast<T*>(this);
			pT->UpdatePanesLayout();
		}
		return lRet;
	}

// Implementation
	BOOL UpdatePanesLayout()
	{
		// get pane positions
		int* pPanesPos = NULL;
		ATLTRY(pPanesPos = (int*)_alloca(m_nPanes * sizeof(int)));
		ATLASSERT(pPanesPos != NULL);
		if(pPanesPos == NULL)
			return FALSE;
		int nRet = GetParts(m_nPanes, pPanesPos);
		ATLASSERT(nRet == m_nPanes);
		if(nRet != m_nPanes)
			return FALSE;
		// calculate offset
		RECT rcClient;
		GetClientRect(&rcClient);
		int cxOff = rcClient.right - (pPanesPos[m_nPanes - 1] + ::GetSystemMetrics(SM_CXVSCROLL) + ::GetSystemMetrics(SM_CXEDGE));
		// find variable width pane
		int i;
		for(i = 0; i < m_nPanes; i++)
		{
			if(m_pPane[i] == ID_DEFAULT_PANE)
				break;
		}
		// resize all panes from the variable one to the right
		if((i < m_nPanes) && (pPanesPos[i] + cxOff) > ((i == 0) ? 0 : pPanesPos[i - 1]))
		{
			for(; i < m_nPanes; i++)
				pPanesPos[i] += cxOff;
		}
		// set pane postions
		return SetParts(m_nPanes, pPanesPos);
	}

	int GetPaneIndexFromID(int nPaneID) const
	{
		for(int i = 0; i < m_nPanes; i++)
		{
			if(m_pPane[i] == nPaneID)
				return i;
		}

		return -1;	// not found
	}
};

class CMultiPaneStatusBarCtrl : public CMultiPaneStatusBarCtrlImpl<CMultiPaneStatusBarCtrl>
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_MultiPaneStatusBar"), GetWndClassName())
};


/////////////////////////////////////////////////////////////////////////////
// CPaneContainer - provides header with title and close button for panes

// pane container extended styles
#define PANECNT_NOCLOSEBUTTON	0x00000001
#define PANECNT_VERTICAL	0x00000002

template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CPaneContainerImpl : public CWindowImpl< T, TBase, TWinTraits >, public CCustomDraw< T >
{
public:
	DECLARE_WND_CLASS_EX(NULL, 0, -1)

// Constants
	enum
	{
		m_cxyBorder = 2,
		m_cxyTextOffset = 4,
		m_cxyBtnOffset = 1,

		m_cchTitle = 80,

		m_cxImageTB = 13,
		m_cyImageTB = 11,
		m_cxyBtnAddTB = 7,

		m_cxToolBar = m_cxImageTB + m_cxyBtnAddTB + m_cxyBorder + m_cxyBtnOffset,

		m_xBtnImageLeft = 6,
		m_yBtnImageTop = 5,
		m_xBtnImageRight = 12,
		m_yBtnImageBottom = 11,

		m_nCloseBtnID = ID_PANE_CLOSE
	};

// Data members
	CToolBarCtrl m_tb;
	CWindow m_wndClient;
	int m_cxyHeader;
	TCHAR m_szTitle[m_cchTitle];
	DWORD m_dwExtendedStyle;	// Pane container specific extended styles


// Constructor
	CPaneContainerImpl() : m_cxyHeader(0), m_dwExtendedStyle(0)
	{
		m_szTitle[0] = 0;
	}

// Attributes
	DWORD GetPaneContainerExtendedStyle() const
	{
		return m_dwExtendedStyle;
	}

	DWORD SetPaneContainerExtendedStyle(DWORD dwExtendedStyle, DWORD dwMask = 0)
	{
		DWORD dwPrevStyle = m_dwExtendedStyle;
		if(dwMask == 0)
			m_dwExtendedStyle = dwExtendedStyle;
		else
			m_dwExtendedStyle = (m_dwExtendedStyle & ~dwMask) | (dwExtendedStyle & dwMask);
		if(m_hWnd != NULL)
		{
			T* pT = static_cast<T*>(this);
			bool bUpdate = false;

			if(((dwPrevStyle & PANECNT_NOCLOSEBUTTON) != 0) && ((dwExtendedStyle & PANECNT_NOCLOSEBUTTON) == 0))	// add close button
			{
				pT->CreateCloseButton();
				bUpdate = true;
			}
			else if(((dwPrevStyle & PANECNT_NOCLOSEBUTTON) == 0) && ((dwExtendedStyle & PANECNT_NOCLOSEBUTTON) != 0))	// remove close button
			{
				pT->DestroyCloseButton();
				bUpdate = true;
			}

			if((dwPrevStyle & PANECNT_VERTICAL) != (dwExtendedStyle & PANECNT_VERTICAL))	// change orientation
			{
				CalcSize();
				bUpdate = true;
			}

			if(bUpdate)
				pT->UpdateLayout();
		}
		return dwPrevStyle;
	}

	HWND GetClient() const
	{
		return m_wndClient;
	}

	HWND SetClient(HWND hWndClient)
	{
		HWND hWndOldClient = m_wndClient;
		m_wndClient = hWndClient;
		if(m_hWnd != NULL)
		{
			T* pT = static_cast<T*>(this);
			pT->UpdateLayout();
		}
		return hWndOldClient;
	}

	BOOL GetTitle(LPTSTR lpstrTitle, int cchLength) const
	{
		ATLASSERT(lpstrTitle != NULL);
		return (lstrcpyn(lpstrTitle, m_szTitle, cchLength) != NULL);
	}

	BOOL SetTitle(LPCTSTR lpstrTitle)
	{
		ATLASSERT(lpstrTitle != NULL);
		BOOL bRet = (lstrcpyn(m_szTitle, lpstrTitle, m_cchTitle) != NULL);
		if(bRet && m_hWnd != NULL)
		{
			T* pT = static_cast<T*>(this);
			pT->UpdateLayout();
		}
		return bRet;
	}

	int GetTitleLength() const
	{
		return lstrlen(m_szTitle);
	}

// Methods
	HWND Create(HWND hWndParent, LPCTSTR lpstrTitle = NULL, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			DWORD dwExStyle = 0, UINT nID = 0, LPVOID lpCreateParam = NULL)
	{
		if(lpstrTitle != NULL)
			lstrcpyn(m_szTitle, lpstrTitle, m_cchTitle);
		return CWindowImpl< T, TBase, TWinTraits >::Create(hWndParent, rcDefault, NULL, dwStyle, dwExStyle, nID, lpCreateParam);
	}

	HWND Create(HWND hWndParent, UINT uTitleID, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			DWORD dwExStyle = 0, UINT nID = 0, LPVOID lpCreateParam = NULL)
	{
		if(uTitleID != 0U)
			::LoadString(_Module.GetResourceInstance(), uTitleID, m_szTitle, m_cchTitle);
		return CWindowImpl< T, TBase, TWinTraits >::Create(hWndParent, rcDefault, NULL, dwStyle, dwExStyle, nID, lpCreateParam);
	}

	BOOL EnableCloseButton(BOOL bEnable)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT;	// avoid level 4 warning
		return (m_tb.m_hWnd != NULL) ? m_tb.EnableButton(pT->m_nCloseBtnID, bEnable) : FALSE;
	}

	void UpdateLayout()
	{
		RECT rcClient;
		GetClientRect(&rcClient);
		T* pT = static_cast<T*>(this);
		pT->UpdateLayout(rcClient.right, rcClient.bottom);
	}

// Message map and handlers
	BEGIN_MSG_MAP(CPaneContainerImpl)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		FORWARD_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		pT->CalcSize();

		if((m_dwExtendedStyle & PANECNT_NOCLOSEBUTTON) == 0)
			pT->CreateCloseButton();

		return 0;
	}

	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		pT->UpdateLayout(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}

	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		if(m_wndClient.m_hWnd != NULL)
			m_wndClient.SetFocus();
		return 0;
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return 1;	// no background needed
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CPaintDC dc(m_hWnd);

		T* pT = static_cast<T*>(this);
		pT->DrawPaneTitle(dc.m_hDC);

		if(m_wndClient.m_hWnd == NULL)	// no client window
			pT->DrawPane(dc.m_hDC);

		return 0;
	}

	LRESULT OnNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		if(m_tb.m_hWnd == NULL)
		{
			bHandled = FALSE;
			return 1;
		}

		T* pT = static_cast<T*>(this);
		LPNMHDR lpnmh = (LPNMHDR)lParam;
		LRESULT lRet = 0;

		// pass toolbar custom draw notifications to the base class
		if(lpnmh->code == NM_CUSTOMDRAW && lpnmh->hwndFrom == m_tb.m_hWnd)
			lRet = CCustomDraw< T >::OnCustomDraw(0, lpnmh, bHandled);
		// tooltip notifications come with the tooltip window handle and button ID,
		// pass them to the parent if we don't handle them
		else if(lpnmh->code == TTN_GETDISPINFO && lpnmh->idFrom == pT->m_nCloseBtnID)
			bHandled = pT->GetToolTipText(lpnmh);
		// only let notifications not from the toolbar go to the parent
		else if(lpnmh->hwndFrom != m_tb.m_hWnd && lpnmh->idFrom != pT->m_nCloseBtnID)
			bHandled = FALSE;

		return lRet;
	}

	LRESULT OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		// if command comes from the close button, substitute HWND of the pane container instead
		if(m_tb.m_hWnd != NULL && (HWND)lParam == m_tb.m_hWnd)
			return ::SendMessage(GetParent(), WM_COMMAND, wParam, (LPARAM)m_hWnd);

		bHandled = FALSE;
		return 1;
	}

// Custom draw overrides
	DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
	{
		return CDRF_NOTIFYITEMDRAW;	// we need per-item notifications
	}

	DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
	{
		CDCHandle dc = lpNMCustomDraw->hdc;
#if (_WIN32_IE >= 0x0400)
		RECT& rc = lpNMCustomDraw->rc;
#else //!(_WIN32_IE >= 0x0400)
		RECT rc;
		m_tb.GetItemRect(0, &rc);
#endif //!(_WIN32_IE >= 0x0400)

		dc.FillRect(&rc, COLOR_3DFACE);

		return CDRF_NOTIFYPOSTPAINT;
	}

	DWORD OnItemPostPaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
	{
		CDCHandle dc = lpNMCustomDraw->hdc;
#if (_WIN32_IE >= 0x0400)
		RECT& rc = lpNMCustomDraw->rc;
#else //!(_WIN32_IE >= 0x0400)
		RECT rc;
		m_tb.GetItemRect(0, &rc);
#endif //!(_WIN32_IE >= 0x0400)

		RECT rcImage = { m_xBtnImageLeft, m_yBtnImageTop, m_xBtnImageRight + 1, m_yBtnImageBottom + 1 };
		::OffsetRect(&rcImage, rc.left, rc.top);
		T* pT = static_cast<T*>(this);

		if((lpNMCustomDraw->uItemState & CDIS_DISABLED) != 0)
		{
			RECT rcShadow = rcImage;
			::OffsetRect(&rcShadow, 1, 1);
			CPen pen1;
			pen1.CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_3DHILIGHT));
			pT->DrawButtonImage(dc, rcShadow, pen1);
			CPen pen2;
			pen2.CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_3DSHADOW));
			pT->DrawButtonImage(dc, rcImage, pen2);
		}
		else
		{
			if((lpNMCustomDraw->uItemState & CDIS_SELECTED) != 0)
				::OffsetRect(&rcImage, 1, 1);
			CPen pen;
			pen.CreatePen(PS_SOLID, 0, ::GetSysColor(COLOR_BTNTEXT));
			pT->DrawButtonImage(dc, rcImage, pen);
		}

		return CDRF_DODEFAULT;	// continue with the default item painting
	}

// Implementation - overrideable methods
	void UpdateLayout(int cxWidth, int cyHeight)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		RECT rect;

		if(IsVertical())
		{
			::SetRect(&rect, 0, 0, m_cxyHeader, cyHeight);
			if(m_tb.m_hWnd != NULL)
				m_tb.SetWindowPos(NULL, m_cxyBorder, m_cxyBorder + m_cxyBtnOffset, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

			if(m_wndClient.m_hWnd != NULL)
				m_wndClient.SetWindowPos(NULL, m_cxyHeader, 0, cxWidth - m_cxyHeader, cyHeight, SWP_NOZORDER);
			else
				rect.right = cxWidth;
		}
		else
		{
			::SetRect(&rect, 0, 0, cxWidth, m_cxyHeader);
			if(m_tb.m_hWnd != NULL)
				m_tb.SetWindowPos(NULL, rect.right - m_cxToolBar, m_cxyBorder + m_cxyBtnOffset, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

			if(m_wndClient.m_hWnd != NULL)
				m_wndClient.SetWindowPos(NULL, 0, m_cxyHeader, cxWidth, cyHeight - m_cxyHeader, SWP_NOZORDER);
			else
				rect.bottom = cyHeight;
		}

		InvalidateRect(&rect);
	}

	void CreateCloseButton()
	{
		ATLASSERT(m_tb.m_hWnd == NULL);
		// create toolbar for the "x" button
		m_tb.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NOMOVEY | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT, 0);
		ATLASSERT(m_tb.IsWindow());

		if(m_tb.m_hWnd != NULL)
		{
			T* pT = static_cast<T*>(this);
			pT;	// avoid level 4 warning

			m_tb.SetButtonStructSize();

			TBBUTTON tbbtn;
			memset(&tbbtn, 0, sizeof(tbbtn));
			tbbtn.idCommand = pT->m_nCloseBtnID;
			tbbtn.fsState = TBSTATE_ENABLED;
			tbbtn.fsStyle = TBSTYLE_BUTTON;
			m_tb.AddButtons(1, &tbbtn);

			m_tb.SetBitmapSize(m_cxImageTB, m_cyImageTB);
			m_tb.SetButtonSize(m_cxImageTB + m_cxyBtnAddTB, m_cyImageTB + m_cxyBtnAddTB);

			if(IsVertical())
				m_tb.SetWindowPos(NULL, m_cxyBorder + m_cxyBtnOffset, m_cxyBorder + m_cxyBtnOffset, m_cxImageTB + m_cxyBtnAddTB, m_cyImageTB + m_cxyBtnAddTB, SWP_NOZORDER | SWP_NOACTIVATE);
			else
				m_tb.SetWindowPos(NULL, 0, 0, m_cxImageTB + m_cxyBtnAddTB, m_cyImageTB + m_cxyBtnAddTB, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
		}
	}

	void DestroyCloseButton()
	{
		if(m_tb.m_hWnd != NULL)
			m_tb.DestroyWindow();
	}

	void CalcSize()
	{
		T* pT = static_cast<T*>(this);
		CFontHandle font = pT->GetTitleFont();
		LOGFONT lf;
		font.GetLogFont(lf);
		if(IsVertical())
		{
			m_cxyHeader = m_cxImageTB + m_cxyBtnAddTB + m_cxyBorder;
		}
		else
		{
			int cyFont = abs(lf.lfHeight) + m_cxyBorder + 2 * m_cxyTextOffset;
			int cyBtn = m_cyImageTB + m_cxyBtnAddTB + m_cxyBorder + 2 * m_cxyBtnOffset;
			m_cxyHeader = max(cyFont, cyBtn);
		}
	}

	HFONT GetTitleFont() const
	{
		return AtlGetDefaultGuiFont();
	}

	BOOL GetToolTipText(LPNMHDR /*lpnmh*/)
	{
		return FALSE;
	}

	void DrawPaneTitle(CDCHandle dc)
	{
		RECT rect;
		GetClientRect(&rect);

		if(IsVertical())
		{
			rect.right = rect.left + m_cxyHeader;
			dc.DrawEdge(&rect, EDGE_ETCHED, BF_LEFT | BF_TOP | BF_BOTTOM | BF_ADJUST);
			dc.FillRect(&rect, COLOR_3DFACE);
		}
		else
		{
			rect.bottom = rect.top + m_cxyHeader;
			dc.DrawEdge(&rect, EDGE_ETCHED, BF_LEFT | BF_TOP | BF_RIGHT | BF_ADJUST);
			dc.FillRect(&rect, COLOR_3DFACE);
			// draw title only for horizontal pane container
			dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
			dc.SetBkMode(TRANSPARENT);
			T* pT = static_cast<T*>(this);
			HFONT hFontOld = dc.SelectFont(pT->GetTitleFont());
			rect.left += m_cxyTextOffset;
			rect.right -= m_cxyTextOffset;
			if(m_tb.m_hWnd != NULL)
				rect.right -= m_cxToolBar;;
			dc.DrawText(m_szTitle, -1, &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
			dc.SelectFont(hFontOld);
		}
	}

	// called only if pane is empty
	void DrawPane(CDCHandle dc)
	{
		RECT rect;
		GetClientRect(&rect);
		if(IsVertical())
			rect.left += m_cxyHeader;
		else
			rect.top += m_cxyHeader;
		if((GetExStyle() & WS_EX_CLIENTEDGE) == 0)
			dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
		dc.FillRect(&rect, COLOR_APPWORKSPACE);
	}

	// drawing helper - draws "x" button image
	void DrawButtonImage(CDCHandle dc, RECT& rcImage, HPEN hPen)
	{
		HPEN hPenOld = dc.SelectPen(hPen);

		dc.MoveTo(rcImage.left, rcImage.top);
		dc.LineTo(rcImage.right, rcImage.bottom);
		dc.MoveTo(rcImage.left + 1, rcImage.top);
		dc.LineTo(rcImage.right + 1, rcImage.bottom);

		dc.MoveTo(rcImage.left, rcImage.bottom - 1);
		dc.LineTo(rcImage.right, rcImage.top - 1);
		dc.MoveTo(rcImage.left + 1, rcImage.bottom - 1);
		dc.LineTo(rcImage.right + 1, rcImage.top - 1);

		dc.SelectPen(hPenOld);
	}

	bool IsVertical() const
	{
		return ((m_dwExtendedStyle & PANECNT_VERTICAL) != 0);
	}
};

class CPaneContainer : public CPaneContainerImpl<CPaneContainer>
{
public:
	DECLARE_WND_CLASS_EX(_T("WTL_PaneContainer"), 0, -1)
};

}; //namespace WTL

#endif // __ATLCTRLX_H__
