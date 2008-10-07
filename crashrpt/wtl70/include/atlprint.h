// Windows Template Library - WTL version 7.0
// Copyright (C) 1997-2002 Microsoft Corporation
// All rights reserved.
//
// This file is a part of the Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.

#ifndef __ATLPRINT_H__
#define __ATLPRINT_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
	#error atlprint.h requires atlapp.h to be included first
#endif

#ifndef __ATLWIN_H__
	#error atlprint.h requires atlwin.h to be included first
#endif


/////////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CPrinterInfo<t_nInfo>
// CPrinterT<t_bManaged>
// CDevModeT<t_bManaged>
// CPrinterDC
// CPrintJobInfo
// CPrintJob
// CPrintPreview
// CPrintPreviewWindowImpl<T, TBase, TWinTraits>
// CPrintPreviewWindow


namespace WTL
{

/////////////////////////////////////////////////////////////////////////////

template <unsigned int t_nInfo>
class _printer_info
{
public:
	typedef void infotype;
};

template <> class _printer_info<1> { public: typedef PRINTER_INFO_1 infotype; };
template <> class _printer_info<2> { public: typedef PRINTER_INFO_2 infotype; };
template <> class _printer_info<3> { public: typedef PRINTER_INFO_3 infotype; };
template <> class _printer_info<4> { public: typedef PRINTER_INFO_4 infotype; };
template <> class _printer_info<5> { public: typedef PRINTER_INFO_5 infotype; };
template <> class _printer_info<6> { public: typedef PRINTER_INFO_6 infotype; };
template <> class _printer_info<7> { public: typedef PRINTER_INFO_7 infotype; };
// these are not in the old (vc6.0) headers
#ifdef _ATL_USE_NEW_PRINTER_INFO
template <> class _printer_info<8> { public: typedef PRINTER_INFO_8 infotype; };
template <> class _printer_info<9> { public: typedef PRINTER_INFO_9 infotype; };
#endif //_ATL_USE_NEW_PRINTER_INFO

//This class wraps all of the PRINTER_INFO_* structures
//and provided by ::GetPrinter.
template <unsigned int t_nInfo>
class CPrinterInfo
{
public:
// Data members
	_printer_info<t_nInfo>::infotype* m_pi;

// Constructor/destructor
	CPrinterInfo() : m_pi(NULL)
	{ }
	CPrinterInfo(HANDLE hPrinter) : m_pi(NULL)
	{
		GetPrinterInfo(hPrinter);
	}
	~CPrinterInfo()
	{
		Cleanup();
	}

// Operations
	bool GetPrinterInfo(HANDLE hPrinter)
	{
		Cleanup();
		return GetPrinterInfoHelper(hPrinter, (BYTE**)&m_pi, t_nInfo);
	}

// Implementation
	void Cleanup()
	{
		delete [] (BYTE*)m_pi;
		m_pi = NULL;
	}
	static bool GetPrinterInfoHelper(HANDLE hPrinter, BYTE** pi, int nIndex)
	{
		ATLASSERT(pi != NULL);
		DWORD dw = 0;
		BYTE* pb = NULL;
		::GetPrinter(hPrinter, nIndex, NULL, 0, &dw);
		if (dw > 0)
		{
			ATLTRY(pb = new BYTE[dw]);
			if (pb != NULL)
			{
				memset(pb, 0, dw);
				DWORD dwNew;
				if (!::GetPrinter(hPrinter, nIndex, pb, dw, &dwNew))
				{
					delete [] pb;
					pb = NULL;
				}
			}
		}
		*pi = pb;
		return (pb != NULL);
	}
};

//Provides a wrapper class for a HANDLE to a printer.
template <bool t_bManaged>
class CPrinterT
{
public:
// Data members
	HANDLE m_hPrinter;

// Constructor/destructor
	CPrinterT(HANDLE hPrinter = NULL) : m_hPrinter(hPrinter)
	{ }

	~CPrinterT()
	{
		ClosePrinter();
	}

// Operations
	CPrinterT& operator=(HANDLE hPrinter)
	{
		if (hPrinter != m_hPrinter)
		{
			ClosePrinter();
			m_hPrinter = hPrinter;
		}
		return *this;
	}

	bool IsNull() const { return (m_hPrinter == NULL); }

	bool OpenPrinter(HANDLE hDevNames, const DEVMODE* pDevMode = NULL)
	{
		bool b = false;
		DEVNAMES* pdn = (DEVNAMES*)::GlobalLock(hDevNames);
		if (pdn != NULL)
		{
			LPTSTR lpszPrinterName = (LPTSTR)pdn + pdn->wDeviceOffset;
			b = OpenPrinter(lpszPrinterName, pDevMode);
			::GlobalUnlock(hDevNames);
		}
		return b;
	}
	bool OpenPrinter(LPCTSTR lpszPrinterName, const DEVMODE* pDevMode = NULL)
	{
		ClosePrinter();
		PRINTER_DEFAULTS pdefs = { NULL, (DEVMODE*)pDevMode, PRINTER_ACCESS_USE };
		::OpenPrinter((LPTSTR) lpszPrinterName, &m_hPrinter, (pDevMode == NULL) ? NULL : &pdefs);

		return (m_hPrinter != NULL);
	}
	bool OpenPrinter(LPCTSTR lpszPrinterName, PRINTER_DEFAULTS* pprintdefs)
	{
		ClosePrinter();
		::OpenPrinter((LPTSTR) lpszPrinterName, &m_hPrinter, pprintdefs);
		return (m_hPrinter != NULL);
	}
	bool OpenDefaultPrinter(const DEVMODE* pDevMode = NULL)
	{
		ClosePrinter();
		TCHAR buffer[512];
		buffer[0] = 0;
		::GetProfileString(_T("windows"), _T("device"), _T(",,,"), buffer, sizeof(buffer));
		int nLen = lstrlen(buffer);
		if (nLen != 0)
		{
			LPTSTR lpsz = buffer;
			while (*lpsz)
			{
				if (*lpsz == ',')
				{
					*lpsz = 0;
					break;
				}
				lpsz = CharNext(lpsz);
			}
			PRINTER_DEFAULTS pdefs = { NULL, (DEVMODE*)pDevMode, PRINTER_ACCESS_USE };
			::OpenPrinter(buffer, &m_hPrinter, (pDevMode == NULL) ? NULL : &pdefs);
		}
		return m_hPrinter != NULL;
	}
	void ClosePrinter()
	{
		if (m_hPrinter != NULL)
		{
			if (t_bManaged)
				::ClosePrinter(m_hPrinter);
			m_hPrinter = NULL;
		}
	}

	bool PrinterProperties(HWND hWnd = NULL)
	{
		if (hWnd == NULL)
			hWnd = ::GetActiveWindow();
		return !!::PrinterProperties(hWnd, m_hPrinter);
	}
	HANDLE CopyToHDEVNAMES() const
	{
		HANDLE h = NULL;
		CPrinterInfo<5> pinfon5;
		CPrinterInfo<2> pinfon2;
		LPTSTR lpszPrinterName = NULL;
		//Some printers fail for PRINTER_INFO_5 in some situations
		if (pinfon5.GetPrinterInfo(m_hPrinter))
			lpszPrinterName = pinfon5.m_pi->pPrinterName;
		else if (pinfon2.GetPrinterInfo(m_hPrinter))
			lpszPrinterName = pinfon2.m_pi->pPrinterName;
		if (lpszPrinterName != NULL)
		{
			int nLen = sizeof(DEVNAMES) + (lstrlen(lpszPrinterName) + 1) * sizeof(TCHAR);
			h = ::GlobalAlloc(GMEM_MOVEABLE, nLen);
			BYTE* pv = (BYTE*)::GlobalLock(h);
			DEVNAMES* pdev = (DEVNAMES*)pv;
			if (pv != NULL)
			{
				memset(pv, 0, nLen);
				pdev->wDeviceOffset = sizeof(DEVNAMES)/sizeof(TCHAR);
				pv = pv + sizeof(DEVNAMES); //now points to end
				lstrcpy((LPTSTR)pv, lpszPrinterName);
				::GlobalUnlock(h);
			}
		}
		return h;
	}
	HDC CreatePrinterDC(const DEVMODE* pdm = NULL)
	{
		CPrinterInfo<5> pinfo5;
		CPrinterInfo<2> pinfo2;
		HDC hDC = NULL;
		LPTSTR lpszPrinterName = NULL;
		//Some printers fail for PRINTER_INFO_5 in some situations
		if (pinfo5.GetPrinterInfo(m_hPrinter))
			lpszPrinterName = pinfo5.m_pi->pPrinterName;
		else if (pinfo2.GetPrinterInfo(m_hPrinter))
			lpszPrinterName = pinfo2.m_pi->pPrinterName;
		if (lpszPrinterName != NULL)
			hDC = ::CreateDC(NULL, lpszPrinterName, NULL, pdm);
		return hDC;
	}
	HDC CreatePrinterIC(const DEVMODE* pdm = NULL)
	{
		CPrinterInfo<5> pinfo5;
		CPrinterInfo<2> pinfo2;
		HDC hDC = NULL;
		LPTSTR lpszPrinterName = NULL;
		//Some printers fail for PRINTER_INFO_5 in some situations
		if (pinfo5.GetPrinterInfo(m_hPrinter))
			lpszPrinterName = pinfo5.m_pi->pPrinterName;
		else if (pinfo2.GetPrinterInfo(m_hPrinter))
			lpszPrinterName = pinfo2.m_pi->pPrinterName;
		if (lpszPrinterName != NULL)
			hDC = ::CreateIC(NULL, lpszPrinterName, NULL, pdm);
		return hDC;
	}

	void Attach(HANDLE hPrinter)
	{
		ClosePrinter();
		m_hPrinter = hPrinter;
	}

	HANDLE Detach()
	{
		HANDLE hPrinter = m_hPrinter;
		m_hPrinter = NULL;
		return hPrinter;
	}
	operator HANDLE() const { return m_hPrinter; }
};

typedef CPrinterT<false>	CPrinterHandle;
typedef CPrinterT<true> 	CPrinter;


template <bool t_bManaged>
class CDevModeT
{
public:
// Data members
	HANDLE m_hDevMode;
	DEVMODE* m_pDevMode;

// Constructor/destructor
	CDevModeT(HANDLE hDevMode = NULL) : m_hDevMode(hDevMode)
	{
		m_pDevMode = (m_hDevMode != NULL) ? (DEVMODE*)::GlobalLock(m_hDevMode) : NULL;
	}
	~CDevModeT()
	{
		Cleanup();
	}

// Operations
	CDevModeT<t_bManaged>& operator=(HANDLE hDevMode)
	{
		Attach(hDevMode);
		return *this;
	}

	void Attach(HANDLE hDevModeNew)
	{
		Cleanup();
		m_hDevMode = hDevModeNew;
		m_pDevMode = (m_hDevMode != NULL) ? (DEVMODE*)::GlobalLock(m_hDevMode) : NULL;
	}

	HANDLE Detach()
	{
		if (m_hDevMode != NULL)
			::GlobalUnlock(m_hDevMode);
		HANDLE hDevMode = m_hDevMode;
		m_hDevMode = NULL;
		return hDevMode;
	}

	bool IsNull() const { return (m_hDevMode == NULL); }

	bool CopyFromPrinter(HANDLE hPrinter)
	{
		CPrinterInfo<2> pinfo;
		bool b = pinfo.GetPrinterInfo(hPrinter);
		if (b)
		 b = CopyFromDEVMODE(pinfo.m_pi->pDevMode);
		return b;
	}
	bool CopyFromDEVMODE(const DEVMODE* pdm)
	{
		if (pdm == NULL)
			return false;
		int nSize = pdm->dmSize + pdm->dmDriverExtra;
		HANDLE h = ::GlobalAlloc(GMEM_MOVEABLE, nSize);
		if (h != NULL)
		{
			void* p = ::GlobalLock(h);
			memcpy(p, pdm, nSize);
			::GlobalUnlock(h);
		}
		Attach(h);
		return (h != NULL);
	}
	bool CopyFromHDEVMODE(HANDLE hdm)
	{
		bool b = false;
		if (hdm != NULL)
		{
			DEVMODE* pdm = (DEVMODE*)GlobalLock(hdm);
			b = CopyFromDEVMODE(pdm);
			GlobalUnlock(hdm);
		}
		return b;
	}
	HANDLE CopyToHDEVMODE()
	{
		if ((m_hDevMode == NULL) || (m_pDevMode == NULL))
			return NULL;
		int nSize = m_pDevMode->dmSize + m_pDevMode->dmDriverExtra;
		HANDLE h = GlobalAlloc(GMEM_MOVEABLE, nSize);
		if (h != NULL)
		{
			void* p = GlobalLock(h);
			memcpy(p, m_pDevMode, nSize);
		}
		return h;
	}
	// If this devmode was for another printer, this will create a new devmode
	// based on the existing devmode, but retargeted at the new printer
	bool UpdateForNewPrinter(HANDLE hPrinter)
	{
		LONG nLen = ::DocumentProperties(NULL, hPrinter, NULL, NULL, NULL, 0);
		DEVMODE* pdm = (DEVMODE*)_alloca(nLen);
		memset(pdm, 0, nLen);
		LONG l = ::DocumentProperties(NULL, hPrinter, NULL, pdm, m_pDevMode, DM_IN_BUFFER | DM_OUT_BUFFER);
		bool b = false;
		if (l == IDOK)
			b = CopyFromDEVMODE(pdm);
		return b;
	}
	bool DocumentProperties(HANDLE hPrinter, HWND hWnd = NULL)
	{
		CPrinterInfo<1> pi;
		pi.GetPrinterInfo(hPrinter);
		if (hWnd == NULL)
			hWnd = ::GetActiveWindow();

		LONG nLen = ::DocumentProperties(hWnd, hPrinter, pi.m_pi->pName, NULL, NULL, 0);
		DEVMODE* pdm = (DEVMODE*)_alloca(nLen);
		memset(pdm, 0, nLen);
		LONG l = ::DocumentProperties(hWnd, hPrinter, pi.m_pi->pName, pdm, m_pDevMode, DM_IN_BUFFER | DM_OUT_BUFFER | DM_PROMPT);
		bool b = false;
		if (l == IDOK)
			b = CopyFromDEVMODE(pdm);
		return b;
	}
	operator HANDLE() const { return m_hDevMode; }
	operator DEVMODE*() const { return m_pDevMode; }

// Implementation
	void Cleanup()
	{
		if (m_hDevMode != NULL)
		{
			GlobalUnlock(m_hDevMode);
			if(t_bManaged)
				GlobalFree(m_hDevMode);
			m_hDevMode = NULL;
		}
	}
};

typedef CDevModeT<false>	CDevModeHandle;
typedef CDevModeT<true> 	CDevMode;


class CPrinterDC : public CDC
{
public:
// Constructors/destructor
	CPrinterDC()
	{
		CPrinter printer;
		printer.OpenDefaultPrinter();
		Attach(printer.CreatePrinterDC());
		ATLASSERT(m_hDC != NULL);
	}
	CPrinterDC(HANDLE hPrinter, const DEVMODE* pdm = NULL)
	{
		CPrinterHandle p;
		p.Attach(hPrinter);
		Attach(p.CreatePrinterDC(pdm));
		ATLASSERT(m_hDC != NULL);
	}
	~CPrinterDC()
	{
		DeleteDC();
	}
};


//Defines callbacks used by CPrintJob (not a COM interface)
class ATL_NO_VTABLE IPrintJobInfo
{
public:
	virtual void BeginPrintJob(HDC hDC) = 0;		// allocate handles needed, etc.
	virtual void EndPrintJob(HDC hDC, bool bAborted) = 0;	// free handles, etc.
	virtual void PrePrintPage(UINT nPage, HDC hDC) = 0;
	virtual bool PrintPage(UINT nPage, HDC hDC) = 0;
	virtual void PostPrintPage(UINT nPage, HDC hDC) = 0;
	// If you want per page devmodes, return the DEVMODE* to use for nPage.
	// You can optimize by only returning a new DEVMODE* when it is different
	// from the one for nLastPage, otherwise return NULL.
	// When nLastPage==0, the current DEVMODE* will be the default passed to
	// StartPrintJob.
	// Note: During print preview, nLastPage will always be "0".
	virtual DEVMODE* GetNewDevModeForPage(UINT nLastPage, UINT nPage) = 0;
	virtual bool IsValidPage(UINT nPage) = 0;
};


// Provides a default implementatin for IPrintJobInfo
// Typically, MI'd into a document or view class
class ATL_NO_VTABLE CPrintJobInfo : public IPrintJobInfo
{
public:
	virtual void BeginPrintJob(HDC /*hDC*/) //allocate handles needed, etc
	{
	}
	virtual void EndPrintJob(HDC /*hDC*/, bool /*bAborted*/)	// free handles, etc
	{
	}
	virtual void PrePrintPage(UINT /*nPage*/, HDC hDC)
	{
		m_nPJState = ::SaveDC(hDC);
	}
	virtual bool PrintPage(UINT /*nPage*/, HDC /*hDC*/) = 0;
	virtual void PostPrintPage(UINT /*nPage*/, HDC hDC)
	{
		RestoreDC(hDC, m_nPJState);
	}
	virtual DEVMODE* GetNewDevModeForPage(UINT /*nLastPage*/, UINT /*nPage*/)
	{
		return NULL;
	}
	virtual bool IsValidPage(UINT /*nPage*/)
	{
		return true;
	}

// Implementation - data
	int m_nPJState;
};


// Wraps a set of tasks for a specific printer (StartDoc/EndDoc)
// Handles aborting, background printing
class CPrintJob
{
public:
// Data members
	CPrinterHandle m_printer;
	IPrintJobInfo* m_pInfo;
	DEVMODE* m_pDefDevMode;
	DOCINFO m_docinfo;
	int m_nJobID;
	bool m_bCancel;
	bool m_bComplete;
	unsigned long m_nStartPage;
	unsigned long m_nEndPage;

// Constructor/destructor
	CPrintJob() : m_nJobID(0), m_bCancel(false), m_bComplete(true)
	{ }

	~CPrintJob()
	{
		ATLASSERT(IsJobComplete()); //premature destruction?
	}

// Operations
	bool IsJobComplete() const
	{
		return m_bComplete;
	}

	bool StartPrintJob(bool bBackground, HANDLE hPrinter, DEVMODE* pDefaultDevMode,
			IPrintJobInfo* pInfo, LPCTSTR lpszDocName, 
			unsigned long nStartPage, unsigned long nEndPage)
	{
		ATLASSERT(m_bComplete); //previous job not done yet?
		if (pInfo == NULL)
			return false;
		memset(&m_docinfo, 0, sizeof(m_docinfo));
		m_docinfo.cbSize = sizeof(m_docinfo);
		m_docinfo.lpszDocName = lpszDocName;
		m_pInfo = pInfo;
		m_nStartPage = nStartPage;
		m_nEndPage = nEndPage;
		m_printer.Attach(hPrinter);
		m_pDefDevMode = pDefaultDevMode;
		m_bComplete = false;
		if (!bBackground)
		{
			m_bComplete = true;
			return StartHelper();
		}

		//Create a thread and return
		DWORD dwThreadID = 0;
		HANDLE hThread = ::CreateThread(NULL, 0, StartProc, (void*)this, 0, &dwThreadID);
		::CloseHandle(hThread);

		return (hThread != NULL);
	}

// Implementation
	static DWORD WINAPI StartProc(void* p)
	{
		CPrintJob* pThis = (CPrintJob*)p;
		pThis->StartHelper();
		pThis->m_bComplete = true;
		return 0;
	}
	bool StartHelper()
	{
		CDC dcPrinter;
		dcPrinter.Attach(m_printer.CreatePrinterDC(m_pDefDevMode));
		if (dcPrinter.IsNull())
			return false;
			
		m_nJobID = ::StartDoc(dcPrinter, &m_docinfo);
		if (m_nJobID <= 0)
			return false;

		m_pInfo->BeginPrintJob(dcPrinter);

		//print all the pages now
		unsigned long nLastPage = 0;
		for (unsigned long nPage = m_nStartPage; nPage <= m_nEndPage; nPage++)
		{
			if (!m_pInfo->IsValidPage(nPage))
				break;
			DEVMODE* pdm = m_pInfo->GetNewDevModeForPage(nLastPage, nPage);
			if (pdm != NULL)
				dcPrinter.ResetDC(pdm);
			dcPrinter.StartPage();
			m_pInfo->PrePrintPage(nPage, dcPrinter);
			if (!m_pInfo->PrintPage(nPage, dcPrinter))
				m_bCancel = true;
			m_pInfo->PostPrintPage(nPage, dcPrinter);
			dcPrinter.EndPage();
			if (m_bCancel)
				break;
			nLastPage = nPage;
		}

		m_pInfo->EndPrintJob(dcPrinter, m_bCancel);
		if (m_bCancel)
			::AbortDoc(dcPrinter);
		else
			::EndDoc(dcPrinter);
		m_nJobID = 0;
		return true;
	}
	// Cancels a print job.	Can be called asynchronously.
	void CancelPrintJob()
	{
		m_bCancel = true;
	}
};


// Adds print preview support to an existing window
class CPrintPreview
{
public:
// Data members
	IPrintJobInfo* m_pInfo;
	CPrinterHandle m_printer;
	CEnhMetaFile m_meta;
	DEVMODE* m_pDefDevMode;
	DEVMODE* m_pCurDevMode;
	SIZE m_sizeCurPhysOffset;

// Constructor
	CPrintPreview() : m_pInfo(NULL), m_pDefDevMode(NULL), m_pCurDevMode(NULL)
	{
		m_sizeCurPhysOffset.cx = 0;
		m_sizeCurPhysOffset.cy = 0;
	}

// Operations
	void SetPrintPreviewInfo(HANDLE hPrinter, DEVMODE* pDefaultDevMode, IPrintJobInfo* pji)
	{
		m_printer.Attach(hPrinter);
		m_pDefDevMode = pDefaultDevMode;
		m_pInfo = pji;
		m_nCurPage = 0;
		m_pCurDevMode = NULL;
	}
	void SetEnhMetaFile(HENHMETAFILE hEMF)
	{
		m_meta = hEMF;
	}
	void SetPage(int nPage)
	{
		if (!m_pInfo->IsValidPage(nPage))
			return;
		m_nCurPage = nPage;
		m_pCurDevMode = m_pInfo->GetNewDevModeForPage(0, nPage);
		if (m_pCurDevMode == NULL)
			m_pCurDevMode = m_pDefDevMode;
		CDC dcPrinter = m_printer.CreatePrinterDC(m_pCurDevMode);

		int iWidth = dcPrinter.GetDeviceCaps(PHYSICALWIDTH); 
		int iHeight = dcPrinter.GetDeviceCaps(PHYSICALHEIGHT); 
		int nLogx = dcPrinter.GetDeviceCaps(LOGPIXELSX);
		int nLogy = dcPrinter.GetDeviceCaps(LOGPIXELSY);

		RECT rcMM = { 0, 0, ::MulDiv(iWidth, 2540, nLogx), ::MulDiv(iHeight, 2540, nLogy) };

		m_sizeCurPhysOffset.cx = dcPrinter.GetDeviceCaps(PHYSICALOFFSETX);
		m_sizeCurPhysOffset.cy = dcPrinter.GetDeviceCaps(PHYSICALOFFSETY);
		
		CEnhMetaFileDC dcMeta(dcPrinter, &rcMM);
		m_pInfo->PrePrintPage(nPage, dcMeta);
		m_pInfo->PrintPage(nPage, dcMeta);
		m_pInfo->PostPrintPage(nPage, dcMeta);
		m_meta.Attach(dcMeta.Close());
	}
	void GetPageRect(RECT& rc, LPRECT prc)
	{
		int x1 = rc.right-rc.left;
		int y1 = rc.bottom - rc.top;
		if ((x1 < 0) || (y1 < 0))
			return;

		CEnhMetaFileInfo emfinfo(m_meta);
		ENHMETAHEADER* pmh = emfinfo.GetEnhMetaFileHeader();

		//Compute whether we are OK vertically or horizontally
		int x2 = pmh->szlDevice.cx;
		int y2 = pmh->szlDevice.cy;
		int y1p = MulDiv(x1, y2, x2);
		int x1p = MulDiv(y1, x2, y2);
		ATLASSERT( (x1p <= x1) || (y1p <= y1));
		if (x1p <= x1)
		{
			prc->left = rc.left + (x1 - x1p)/2;
			prc->right = prc->left + x1p;
			prc->top = rc.top;
			prc->bottom = rc.bottom;
		}
		else
		{
			prc->left = rc.left;
			prc->right = rc.right;
			prc->top = rc.top + (y1 - y1p)/2;
			prc->bottom = prc->top + y1p;
		}
	}

// Painting helper
	void DoPaint(CDCHandle dc, RECT& rc)
	{
		CEnhMetaFileInfo emfinfo(m_meta);
		ENHMETAHEADER* pmh = emfinfo.GetEnhMetaFileHeader();
		int nOffsetX = MulDiv(m_sizeCurPhysOffset.cx, rc.right-rc.left, pmh->szlDevice.cx);
		int nOffsetY = MulDiv(m_sizeCurPhysOffset.cy, rc.bottom-rc.top, pmh->szlDevice.cy);

		dc.OffsetWindowOrg(-nOffsetX, -nOffsetY);
		dc.PlayMetaFile(m_meta, &rc);
	}

// Implementation - data
	int m_nCurPage;
};

template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CPrintPreviewWindowImpl : public CWindowImpl<T, TBase, TWinTraits>, public CPrintPreview
{
public:
	DECLARE_WND_CLASS_EX(NULL, CS_VREDRAW | CS_HREDRAW, -1)

	enum { m_cxOffset = 10, m_cyOffset = 10 };

// Constructor
	CPrintPreviewWindowImpl() : m_nMaxPage(0), m_nMinPage(0)
	{ }

// Operations
	void SetPrintPreviewInfo(HANDLE hPrinter, DEVMODE* pDefaultDevMode, 
		IPrintJobInfo* pji, int nMinPage, int nMaxPage)
	{
		CPrintPreview::SetPrintPreviewInfo(hPrinter, pDefaultDevMode, pji);
		m_nMinPage = nMinPage;
		m_nMaxPage = nMaxPage;
	}
	bool NextPage()
	{
		if (m_nCurPage == m_nMaxPage)
			return false;
		SetPage(m_nCurPage + 1);
		Invalidate();
		return true;
	}
	bool PrevPage()
	{
		if (m_nCurPage == m_nMinPage)
			return false;
		if (m_nCurPage == 0)
			return false;
		SetPage(m_nCurPage - 1);
		Invalidate();
		return true;
	}

// Message map and handlers
	BEGIN_MSG_MAP(CPrintPreviewWindowImpl)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
	END_MSG_MAP()

	LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return 1;	// no need for the background
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		CPaintDC dc(m_hWnd);
		RECT rcClient;
		GetClientRect(&rcClient);
		RECT rcArea = rcClient;
		::InflateRect(&rcArea, -pT->m_cxOffset, -pT->m_cyOffset);
		if (rcArea.left > rcArea.right)
			rcArea.right = rcArea.left;
		if (rcArea.top > rcArea.bottom)
			rcArea.bottom = rcArea.top;
		RECT rc;
		GetPageRect(rcArea, &rc);
		CRgn rgn1, rgn2;
		rgn1.CreateRectRgnIndirect(&rc);
		rgn2.CreateRectRgnIndirect(&rcClient);
		rgn2.CombineRgn(rgn1, RGN_DIFF);
		dc.SelectClipRgn(rgn2);
		dc.FillRect(&rcClient, COLOR_BTNSHADOW);
		dc.SelectClipRgn(NULL);
		dc.FillRect(&rc, (HBRUSH)::GetStockObject(WHITE_BRUSH));
		pT->DoPaint(dc.m_hDC, rc);
		return 0;
	}

// Implementation - data
	int m_nMinPage;
	int m_nMaxPage;
};


class CPrintPreviewWindow : public CPrintPreviewWindowImpl<CPrintPreviewWindow>
{
public:
	DECLARE_WND_CLASS_EX(_T("WTL_PrintPreview"), CS_VREDRAW | CS_HREDRAW, -1)
};

}; //namespace WTL

#endif // __ATLPRINT_H__
