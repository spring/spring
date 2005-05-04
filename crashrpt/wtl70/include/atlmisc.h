// Windows Template Library - WTL version 7.0
// Copyright (C) 1997-2002 Microsoft Corporation
// All rights reserved.
//
// This file is a part of the Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.

#ifndef __ATLMISC_H__
#define __ATLMISC_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
	#error atlmisc.h requires atlapp.h to be included first
#endif


#ifdef _ATL_TMP_NO_CSTRING
#define _WTL_NO_CSTRING
#endif

#if defined(_WTL_USE_CSTRING) && defined(_WTL_NO_CSTRING)
	#error Conflicting options - both _WTL_USE_CSTRING and _WTL_NO_CSTRING are defined
#endif //defined(_WTL_USE_CSTRING) && defined(_WTL_NO_CSTRING)

#if !defined(_WTL_USE_CSTRING) && !defined(_WTL_NO_CSTRING)
#define _WTL_USE_CSTRING
#endif //!defined(_WTL_USE_CSTRING) && !defined(_WTL_NO_CSTRING)

#ifndef _WTL_NO_CSTRING
  #if defined(_ATL_USE_CSTRING_FLOAT) && defined(_ATL_MIN_CRT)
	#error Cannot use CString floating point formatting with _ATL_MIN_CRT defined
  #endif //defined(_ATL_USE_CSTRING_FLOAT) && defined(_ATL_MIN_CRT)

  #ifndef _DEBUG
  #include <stdio.h>
  #endif //!_DEBUG
#endif //!_WTL_NO_CSTRING


/////////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CSize
// CPoint
// CRect
// CString
//
// CRecentDocumentListBase<T, t_cchItemLen>
// CRecentDocumentList
// CFindFile


namespace WTL
{

#ifndef _WTL_NO_WTYPES

// forward declarations
class CSize;
class CPoint;
class CRect;

/////////////////////////////////////////////////////////////////////////////
// CSize - Wrapper for Windows SIZE structure.

class CSize : public tagSIZE
{
public:
// Constructors
	CSize();
	CSize(int initCX, int initCY);
	CSize(SIZE initSize);
	CSize(POINT initPt);
	CSize(DWORD dwSize);

// Operations
	BOOL operator==(SIZE size) const;
	BOOL operator!=(SIZE size) const;
	void operator+=(SIZE size);
	void operator-=(SIZE size);
	void SetSize(int CX, int CY);

// Operators returning CSize values
	CSize operator+(SIZE size) const;
	CSize operator-(SIZE size) const;
	CSize operator-() const;

// Operators returning CPoint values
	CPoint operator+(POINT point) const;
	CPoint operator-(POINT point) const;

// Operators returning CRect values
	CRect operator+(const RECT* lpRect) const;
	CRect operator-(const RECT* lpRect) const;
};


/////////////////////////////////////////////////////////////////////////////
// CPoint - Wrapper for Windows POINT structure.

class CPoint : public tagPOINT
{
public:
// Constructors
	CPoint();
	CPoint(int initX, int initY);
	CPoint(POINT initPt);
	CPoint(SIZE initSize);
	CPoint(DWORD dwPoint);

// Operations
	void Offset(int xOffset, int yOffset);
	void Offset(POINT point);
	void Offset(SIZE size);
	BOOL operator==(POINT point) const;
	BOOL operator!=(POINT point) const;
	void operator+=(SIZE size);
	void operator-=(SIZE size);
	void operator+=(POINT point);
	void operator-=(POINT point);
	void SetPoint(int X, int Y);

// Operators returning CPoint values
	CPoint operator+(SIZE size) const;
	CPoint operator-(SIZE size) const;
	CPoint operator-() const;
	CPoint operator+(POINT point) const;

// Operators returning CSize values
	CSize operator-(POINT point) const;

// Operators returning CRect values
	CRect operator+(const RECT* lpRect) const;
	CRect operator-(const RECT* lpRect) const;
};


/////////////////////////////////////////////////////////////////////////////
// CRect - Wrapper for Windows RECT structure.

class CRect : public tagRECT
{
public:
// Constructors
	CRect();
	CRect(int l, int t, int r, int b);
	CRect(const RECT& srcRect);
	CRect(LPCRECT lpSrcRect);
	CRect(POINT point, SIZE size);
	CRect(POINT topLeft, POINT bottomRight);

// Attributes (in addition to RECT members)
	int Width() const;
	int Height() const;
	CSize Size() const;
	CPoint& TopLeft();
	CPoint& BottomRight();
	const CPoint& TopLeft() const;
	const CPoint& BottomRight() const;
	CPoint CenterPoint() const;

	// convert between CRect and LPRECT/LPCRECT (no need for &)
	operator LPRECT();
	operator LPCRECT() const;

	BOOL IsRectEmpty() const;
	BOOL IsRectNull() const;
	BOOL PtInRect(POINT point) const;

// Operations
	void SetRect(int x1, int y1, int x2, int y2);
	void SetRect(POINT topLeft, POINT bottomRight);
	void SetRectEmpty();
	void CopyRect(LPCRECT lpSrcRect);
	BOOL EqualRect(LPCRECT lpRect) const;

	void InflateRect(int x, int y);
	void InflateRect(SIZE size);
	void InflateRect(LPCRECT lpRect);
	void InflateRect(int l, int t, int r, int b);
	void DeflateRect(int x, int y);
	void DeflateRect(SIZE size);
	void DeflateRect(LPCRECT lpRect);
	void DeflateRect(int l, int t, int r, int b);

	void OffsetRect(int x, int y);
	void OffsetRect(SIZE size);
	void OffsetRect(POINT point);
	void NormalizeRect();

	// absolute position of rectangle
	void MoveToY(int y);
	void MoveToX(int x);
	void MoveToXY(int x, int y);
	void MoveToXY(POINT point);

	// operations that fill '*this' with result
	BOOL IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2);
	BOOL UnionRect(LPCRECT lpRect1, LPCRECT lpRect2);
	BOOL SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2);

// Additional Operations
	void operator=(const RECT& srcRect);
	BOOL operator==(const RECT& rect) const;
	BOOL operator!=(const RECT& rect) const;
	void operator+=(POINT point);
	void operator+=(SIZE size);
	void operator+=(LPCRECT lpRect);
	void operator-=(POINT point);
	void operator-=(SIZE size);
	void operator-=(LPCRECT lpRect);
	void operator&=(const RECT& rect);
	void operator|=(const RECT& rect);

// Operators returning CRect values
	CRect operator+(POINT point) const;
	CRect operator-(POINT point) const;
	CRect operator+(LPCRECT lpRect) const;
	CRect operator+(SIZE size) const;
	CRect operator-(SIZE size) const;
	CRect operator-(LPCRECT lpRect) const;
	CRect operator&(const RECT& rect2) const;
	CRect operator|(const RECT& rect2) const;
	CRect MulDiv(int nMultiplier, int nDivisor) const;
};


/////////////////////////////////////////////////////////////////////////////
// CSize, CPoint, CRect Implementation

// CSize
inline CSize::CSize()
	{ /* random filled */ }
inline CSize::CSize(int initCX, int initCY)
	{ cx = initCX; cy = initCY; }
inline CSize::CSize(SIZE initSize)
	{ *(SIZE*)this = initSize; }
inline CSize::CSize(POINT initPt)
	{ *(POINT*)this = initPt; }
inline CSize::CSize(DWORD dwSize)
	{
		cx = (short)LOWORD(dwSize);
		cy = (short)HIWORD(dwSize);
	}
inline BOOL CSize::operator==(SIZE size) const
	{ return (cx == size.cx && cy == size.cy); }
inline BOOL CSize::operator!=(SIZE size) const
	{ return (cx != size.cx || cy != size.cy); }
inline void CSize::operator+=(SIZE size)
	{ cx += size.cx; cy += size.cy; }
inline void CSize::operator-=(SIZE size)
	{ cx -= size.cx; cy -= size.cy; }
inline void CSize::SetSize(int CX, int CY)
	{ cx = CX; cy = CY; }	
inline CSize CSize::operator+(SIZE size) const
	{ return CSize(cx + size.cx, cy + size.cy); }
inline CSize CSize::operator-(SIZE size) const
	{ return CSize(cx - size.cx, cy - size.cy); }
inline CSize CSize::operator-() const
	{ return CSize(-cx, -cy); }
inline CPoint CSize::operator+(POINT point) const
	{ return CPoint(cx + point.x, cy + point.y); }
inline CPoint CSize::operator-(POINT point) const
	{ return CPoint(cx - point.x, cy - point.y); }
inline CRect CSize::operator+(const RECT* lpRect) const
	{ return CRect(lpRect) + *this; }
inline CRect CSize::operator-(const RECT* lpRect) const
	{ return CRect(lpRect) - *this; }

// CPoint
inline CPoint::CPoint()
	{ /* random filled */ }
inline CPoint::CPoint(int initX, int initY)
	{ x = initX; y = initY; }
inline CPoint::CPoint(POINT initPt)
	{ *(POINT*)this = initPt; }
inline CPoint::CPoint(SIZE initSize)
	{ *(SIZE*)this = initSize; }
inline CPoint::CPoint(DWORD dwPoint)
	{
		x = (short)LOWORD(dwPoint);
		y = (short)HIWORD(dwPoint);
	}
inline void CPoint::Offset(int xOffset, int yOffset)
	{ x += xOffset; y += yOffset; }
inline void CPoint::Offset(POINT point)
	{ x += point.x; y += point.y; }
inline void CPoint::Offset(SIZE size)
	{ x += size.cx; y += size.cy; }
inline BOOL CPoint::operator==(POINT point) const
	{ return (x == point.x && y == point.y); }
inline BOOL CPoint::operator!=(POINT point) const
	{ return (x != point.x || y != point.y); }
inline void CPoint::operator+=(SIZE size)
	{ x += size.cx; y += size.cy; }
inline void CPoint::operator-=(SIZE size)
	{ x -= size.cx; y -= size.cy; }
inline void CPoint::operator+=(POINT point)
	{ x += point.x; y += point.y; }
inline void CPoint::operator-=(POINT point)
	{ x -= point.x; y -= point.y; }
inline void CPoint::SetPoint(int X, int Y)
	{ x = X; y = Y; }
inline CPoint CPoint::operator+(SIZE size) const
	{ return CPoint(x + size.cx, y + size.cy); }
inline CPoint CPoint::operator-(SIZE size) const
	{ return CPoint(x - size.cx, y - size.cy); }
inline CPoint CPoint::operator-() const
	{ return CPoint(-x, -y); }
inline CPoint CPoint::operator+(POINT point) const
	{ return CPoint(x + point.x, y + point.y); }
inline CSize CPoint::operator-(POINT point) const
	{ return CSize(x - point.x, y - point.y); }
inline CRect CPoint::operator+(const RECT* lpRect) const
	{ return CRect(lpRect) + *this; }
inline CRect CPoint::operator-(const RECT* lpRect) const
	{ return CRect(lpRect) - *this; }

// CRect
inline CRect::CRect()
	{ /* random filled */ }
inline CRect::CRect(int l, int t, int r, int b)
	{ left = l; top = t; right = r; bottom = b; }
inline CRect::CRect(const RECT& srcRect)
	{ ::CopyRect(this, &srcRect); }
inline CRect::CRect(LPCRECT lpSrcRect)
	{ ::CopyRect(this, lpSrcRect); }
inline CRect::CRect(POINT point, SIZE size)
	{ right = (left = point.x) + size.cx; bottom = (top = point.y) + size.cy; }
inline CRect::CRect(POINT topLeft, POINT bottomRight)
	{ left = topLeft.x; top = topLeft.y;
		right = bottomRight.x; bottom = bottomRight.y; }
inline int CRect::Width() const
	{ return right - left; }
inline int CRect::Height() const
	{ return bottom - top; }
inline CSize CRect::Size() const
	{ return CSize(right - left, bottom - top); }
inline CPoint& CRect::TopLeft()
	{ return *((CPoint*)this); }
inline CPoint& CRect::BottomRight()
	{ return *((CPoint*)this + 1); }
inline const CPoint& CRect::TopLeft() const
	{ return *((CPoint*)this); }
inline const CPoint& CRect::BottomRight() const
	{ return *((CPoint*)this + 1); }
inline CPoint CRect::CenterPoint() const
	{ return CPoint((left + right) / 2, (top + bottom) / 2); }
inline CRect::operator LPRECT()
	{ return this; }
inline CRect::operator LPCRECT() const
	{ return this; }
inline BOOL CRect::IsRectEmpty() const
	{ return ::IsRectEmpty(this); }
inline BOOL CRect::IsRectNull() const
	{ return (left == 0 && right == 0 && top == 0 && bottom == 0); }
inline BOOL CRect::PtInRect(POINT point) const
	{ return ::PtInRect(this, point); }
inline void CRect::SetRect(int x1, int y1, int x2, int y2)
	{ ::SetRect(this, x1, y1, x2, y2); }
inline void CRect::SetRect(POINT topLeft, POINT bottomRight)
	{ ::SetRect(this, topLeft.x, topLeft.y, bottomRight.x, bottomRight.y); }
inline void CRect::SetRectEmpty()
	{ ::SetRectEmpty(this); }
inline void CRect::CopyRect(LPCRECT lpSrcRect)
	{ ::CopyRect(this, lpSrcRect); }
inline BOOL CRect::EqualRect(LPCRECT lpRect) const
	{ return ::EqualRect(this, lpRect); }
inline void CRect::InflateRect(int x, int y)
	{ ::InflateRect(this, x, y); }
inline void CRect::InflateRect(SIZE size)
	{ ::InflateRect(this, size.cx, size.cy); }
inline void CRect::DeflateRect(int x, int y)
	{ ::InflateRect(this, -x, -y); }
inline void CRect::DeflateRect(SIZE size)
	{ ::InflateRect(this, -size.cx, -size.cy); }
inline void CRect::OffsetRect(int x, int y)
	{ ::OffsetRect(this, x, y); }
inline void CRect::OffsetRect(POINT point)
	{ ::OffsetRect(this, point.x, point.y); }
inline void CRect::OffsetRect(SIZE size)
	{ ::OffsetRect(this, size.cx, size.cy); }
inline BOOL CRect::IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2)
	{ return ::IntersectRect(this, lpRect1, lpRect2);}
inline BOOL CRect::UnionRect(LPCRECT lpRect1, LPCRECT lpRect2)
	{ return ::UnionRect(this, lpRect1, lpRect2); }
inline void CRect::operator=(const RECT& srcRect)
	{ ::CopyRect(this, &srcRect); }
inline BOOL CRect::operator==(const RECT& rect) const
	{ return ::EqualRect(this, &rect); }
inline BOOL CRect::operator!=(const RECT& rect) const
	{ return !::EqualRect(this, &rect); }
inline void CRect::operator+=(POINT point)
	{ ::OffsetRect(this, point.x, point.y); }
inline void CRect::operator+=(SIZE size)
	{ ::OffsetRect(this, size.cx, size.cy); }
inline void CRect::operator+=(LPCRECT lpRect)
	{ InflateRect(lpRect); }
inline void CRect::operator-=(POINT point)
	{ ::OffsetRect(this, -point.x, -point.y); }
inline void CRect::operator-=(SIZE size)
	{ ::OffsetRect(this, -size.cx, -size.cy); }
inline void CRect::operator-=(LPCRECT lpRect)
	{ DeflateRect(lpRect); }
inline void CRect::operator&=(const RECT& rect)
	{ ::IntersectRect(this, this, &rect); }
inline void CRect::operator|=(const RECT& rect)
	{ ::UnionRect(this, this, &rect); }
inline CRect CRect::operator+(POINT pt) const
	{ CRect rect(*this); ::OffsetRect(&rect, pt.x, pt.y); return rect; }
inline CRect CRect::operator-(POINT pt) const
	{ CRect rect(*this); ::OffsetRect(&rect, -pt.x, -pt.y); return rect; }
inline CRect CRect::operator+(SIZE size) const
	{ CRect rect(*this); ::OffsetRect(&rect, size.cx, size.cy); return rect; }
inline CRect CRect::operator-(SIZE size) const
	{ CRect rect(*this); ::OffsetRect(&rect, -size.cx, -size.cy); return rect; }
inline CRect CRect::operator+(LPCRECT lpRect) const
	{ CRect rect(this); rect.InflateRect(lpRect); return rect; }
inline CRect CRect::operator-(LPCRECT lpRect) const
	{ CRect rect(this); rect.DeflateRect(lpRect); return rect; }
inline CRect CRect::operator&(const RECT& rect2) const
	{ CRect rect; ::IntersectRect(&rect, this, &rect2);
		return rect; }
inline CRect CRect::operator|(const RECT& rect2) const
	{ CRect rect; ::UnionRect(&rect, this, &rect2);
		return rect; }
inline BOOL CRect::SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2)
	{ return ::SubtractRect(this, lpRectSrc1, lpRectSrc2); }

inline void CRect::NormalizeRect()
{
	int nTemp;
	if (left > right)
	{
		nTemp = left;
		left = right;
		right = nTemp;
	}
	if (top > bottom)
	{
		nTemp = top;
		top = bottom;
		bottom = nTemp;
	}
}

inline void CRect::MoveToY(int y)
	{ bottom = Height() + y; top = y; }
inline void CRect::MoveToX(int x)
	{ right = Width() + x; left = x; }
inline void CRect::MoveToXY(int x, int y)
	{ MoveToX(x); MoveToY(y); }
inline void CRect::MoveToXY(POINT pt)
	{ MoveToX(pt.x); MoveToY(pt.y); }

inline void CRect::InflateRect(LPCRECT lpRect)
{
	left -= lpRect->left;
	top -= lpRect->top;
	right += lpRect->right;
	bottom += lpRect->bottom;
}

inline void CRect::InflateRect(int l, int t, int r, int b)
{
	left -= l;
	top -= t;
	right += r;
	bottom += b;
}

inline void CRect::DeflateRect(LPCRECT lpRect)
{
	left += lpRect->left;
	top += lpRect->top;
	right -= lpRect->right;
	bottom -= lpRect->bottom;
}

inline void CRect::DeflateRect(int l, int t, int r, int b)
{
	left += l;
	top += t;
	right -= r;
	bottom -= b;
}

inline CRect CRect::MulDiv(int nMultiplier, int nDivisor) const
{
	return CRect(
		::MulDiv(left, nMultiplier, nDivisor),
		::MulDiv(top, nMultiplier, nDivisor),
		::MulDiv(right, nMultiplier, nDivisor),
		::MulDiv(bottom, nMultiplier, nDivisor));
}

#endif //!_WTL_NO_WTYPES


/////////////////////////////////////////////////////////////////////////////
// CString - String class

#ifndef _WTL_NO_CSTRING

struct CStringData
{
	long nRefs;     // reference count
	int nDataLength;
	int nAllocLength;
	// TCHAR data[nAllocLength]

	TCHAR* data()
		{ return (TCHAR*)(this + 1); }
};

// Globals

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
_declspec(selectany) int rgInitData[] = { -1, 0, 0, 0 };
_declspec(selectany) CStringData* _atltmpDataNil = (CStringData*)&rgInitData;
_declspec(selectany) LPCTSTR _atltmpPchNil = (LPCTSTR)(((BYTE*)&rgInitData) + sizeof(CStringData));


class CString
{
public:
// Constructors
	CString();
	CString(const CString& stringSrc);
	CString(TCHAR ch, int nRepeat = 1);
	CString(LPCSTR lpsz);
	CString(LPCWSTR lpsz);
	CString(LPCSTR lpch, int nLength);
	CString(LPCWSTR lpch, int nLength);
	CString(const unsigned char* psz);

// Attributes & Operations
	// as an array of characters
	int GetLength() const;
	BOOL IsEmpty() const;
	void Empty();                       // free up the data

	TCHAR GetAt(int nIndex) const;      // 0 based
	TCHAR operator[](int nIndex) const; // same as GetAt
	void SetAt(int nIndex, TCHAR ch);
	operator LPCTSTR() const;           // as a C string

	// overloaded assignment
	const CString& operator=(const CString& stringSrc);
	const CString& operator=(TCHAR ch);
#ifdef _UNICODE
	const CString& operator=(char ch);
#endif
	const CString& operator=(LPCSTR lpsz);
	const CString& operator=(LPCWSTR lpsz);
	const CString& operator=(const unsigned char* psz);

	// string concatenation
	const CString& operator+=(const CString& string);
	const CString& operator+=(TCHAR ch);
#ifdef _UNICODE
	const CString& operator+=(char ch);
#endif
	const CString& operator+=(LPCTSTR lpsz);

	friend CString __stdcall operator+(const CString& string1, const CString& string2);
	friend CString __stdcall operator+(const CString& string, TCHAR ch);
	friend CString __stdcall operator+(TCHAR ch, const CString& string);
#ifdef _UNICODE
	friend CString __stdcall operator+(const CString& string, char ch);
	friend CString __stdcall operator+(char ch, const CString& string);
#endif
	friend CString __stdcall operator+(const CString& string, LPCTSTR lpsz);
	friend CString __stdcall operator+(LPCTSTR lpsz, const CString& string);

	// string comparison
	int Compare(LPCTSTR lpsz) const;         // straight character
	int CompareNoCase(LPCTSTR lpsz) const;   // ignore case
	int Collate(LPCTSTR lpsz) const;         // NLS aware
	int CollateNoCase(LPCTSTR lpsz) const;   // ignore case

	// simple sub-string extraction
	CString Mid(int nFirst, int nCount) const;
	CString Mid(int nFirst) const;
	CString Left(int nCount) const;
	CString Right(int nCount) const;

	CString SpanIncluding(LPCTSTR lpszCharSet) const;
	CString SpanExcluding(LPCTSTR lpszCharSet) const;

	// upper/lower/reverse conversion
	void MakeUpper();
	void MakeLower();
	void MakeReverse();

	// trimming whitespace (either side)
	void TrimRight();
	void TrimLeft();

	// remove continuous occurrences of chTarget starting from right
	void TrimRight(TCHAR chTarget);
	// remove continuous occcurrences of characters in passed string,
	// starting from right
	void TrimRight(LPCTSTR lpszTargets);
	// remove continuous occurrences of chTarget starting from left
	void TrimLeft(TCHAR chTarget);
	// remove continuous occcurrences of characters in
	// passed string, starting from left
	void TrimLeft(LPCTSTR lpszTargets);

	// advanced manipulation
	// replace occurrences of chOld with chNew
	int Replace(TCHAR chOld, TCHAR chNew);
	// replace occurrences of substring lpszOld with lpszNew;
	// empty lpszNew removes instances of lpszOld
	int Replace(LPCTSTR lpszOld, LPCTSTR lpszNew);
	// remove occurrences of chRemove
	int Remove(TCHAR chRemove);
	// insert character at zero-based index; concatenates
	// if index is past end of string
	int Insert(int nIndex, TCHAR ch);
	// insert substring at zero-based index; concatenates
	// if index is past end of string
	int Insert(int nIndex, LPCTSTR pstr);
	// delete nCount characters starting at zero-based index
	int Delete(int nIndex, int nCount = 1);

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(TCHAR ch) const;                     // like "C" strchr
	int ReverseFind(TCHAR ch) const;
	int Find(TCHAR ch, int nStart) const;         // starting at index
	int FindOneOf(LPCTSTR lpszCharSet) const;

	// look for a specific sub-string
	int Find(LPCTSTR lpszSub) const;        // like "C" strstr
	int Find(LPCTSTR lpszSub, int nStart) const;  // starting at index

	// Concatentation for non strings
	const CString& Append(int n)
	{
		TCHAR szBuffer[10];
		wsprintf(szBuffer,_T("%d"),n);
		ConcatInPlace(SafeStrlen(szBuffer), szBuffer);
		return *this;
	}

	// simple formatting
	BOOL __cdecl Format(LPCTSTR lpszFormat, ...);
	BOOL __cdecl Format(UINT nFormatID, ...);
	BOOL FormatV(LPCTSTR lpszFormat, va_list argList);

	// formatting for localization (uses FormatMessage API)
	BOOL __cdecl FormatMessage(LPCTSTR lpszFormat, ...);
	BOOL __cdecl FormatMessage(UINT nFormatID, ...);

	// Windows support
	BOOL LoadString(UINT nID);          // load from string resource
										// 255 chars max
#ifndef _UNICODE
	// ANSI <-> OEM support (convert string in place)
	void AnsiToOem();
	void OemToAnsi();
#endif

#ifndef _ATL_NO_COM
	// OLE BSTR support (use for OLE automation)
	BSTR AllocSysString() const;
	BSTR SetSysString(BSTR* pbstr) const;
#endif //!_ATL_NO_COM

	// Access to string implementation buffer as "C" character array
	LPTSTR GetBuffer(int nMinBufLength);
	void ReleaseBuffer(int nNewLength = -1);
	LPTSTR GetBufferSetLength(int nNewLength);
	void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off
	LPTSTR LockBuffer();
	void UnlockBuffer();

// Implementation
public:
	~CString();
	int GetAllocLength() const;

	static BOOL __stdcall _IsValidString(LPCWSTR lpsz, int nLength = -1)
	{
		if(lpsz == NULL)
			return FALSE;
		return !::IsBadStringPtrW(lpsz, nLength);
	}

	static BOOL __stdcall _IsValidString(LPCSTR lpsz, int nLength = -1)
	{
		if(lpsz == NULL)
			return FALSE;
		return !::IsBadStringPtrA(lpsz, nLength);
	}

protected:
	LPTSTR m_pchData;   // pointer to ref counted string data

	// implementation helpers
	CStringData* GetData() const;
	void Init();
	void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	BOOL AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
	BOOL ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data, int nSrc2Len, LPCTSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData);
	void CopyBeforeWrite();
	BOOL AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(CStringData* pData);
	static int PASCAL SafeStrlen(LPCTSTR lpsz);

	static int __stdcall _LoadString(UINT nID, LPTSTR lpszBuf, UINT nMaxBuf)
	{
#ifdef _DEBUG
		// LoadString without annoying warning from the Debug kernel if the
		//  segment containing the string is not present
		if (::FindResource(_Module.GetResourceInstance(), MAKEINTRESOURCE((nID>>4) + 1), RT_STRING) == NULL)
		{
			lpszBuf[0] = '\0';
			return 0; // not found
		}
#endif //_DEBUG
		int nLen = ::LoadString(_Module.GetResourceInstance(), nID, lpszBuf, nMaxBuf);
		if (nLen == 0)
			lpszBuf[0] = '\0';
		return nLen;
	}

	static const CString& __stdcall _GetEmptyString()
	{
		return *(CString*)&_atltmpPchNil;
	}

// CString conversion helpers
	static int __cdecl _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
	{
		if (count == 0 && mbstr != NULL)
			return 0;

		int result = ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1, mbstr, (int)count, NULL, NULL);
		ATLASSERT(mbstr == NULL || result <= (int)count);
		if (result > 0)
			mbstr[result - 1] = 0;
		return result;
	}

	static int __cdecl _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
	{
		if (count == 0 && wcstr != NULL)
			return 0;

		int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1, wcstr, (int)count);
		ATLASSERT(wcstr == NULL || result <= (int)count);
		if (result > 0)
			wcstr[result - 1] = 0;
		return result;
	}

// Helpers to avoid CRT startup code
#ifdef _ATL_MIN_CRT
	static TCHAR* _cstrchr(const TCHAR* p, TCHAR ch)
	{
		//strchr for '\0' should succeed
		while (*p != 0)
		{
			if (*p == ch)
				break;
			p = ::CharNext(p);
		}
		return (TCHAR*)((*p == ch) ? p : NULL);
	}
	static TCHAR* _cstrchr_db(const TCHAR* p, TCHAR ch1, TCHAR ch2)
	{
		const TCHAR* lpsz = NULL;
		while (*p != 0)
		{
			if (*p == ch1 && *(p + 1) == ch2)
			{
				lpsz = p;
				break;
			}
			p = ::CharNext(p);
		}
		return (TCHAR*)lpsz;
	}
	static TCHAR* _cstrrchr(const TCHAR* p, TCHAR ch)
	{
		const TCHAR* lpsz = NULL;
		while (*p != 0)
		{
			if (*p == ch)
				lpsz = p;
			p = ::CharNext(p);
		}
		return (TCHAR*)lpsz;
	}
	static TCHAR* _cstrrev(TCHAR* pStr)
	{
		// Optimize NULL, zero-length, and single-char case.
		if ((pStr == NULL) || (pStr[0] == '\0') || (pStr[1] == '\0'))
			return pStr;

		TCHAR* p = pStr;

		while (p[1] != 0) 
		{
			TCHAR* pNext = ::CharNext(p);
			if(pNext > p + 1)
			{
				char p1 = *(char*)p;
				*(char*)p = *(char*)(p + 1);
				*(char*)(p + 1) = p1;
			}
			p = pNext;
		}

		TCHAR* q = pStr;

		while (q < p)
		{
			TCHAR t = *q;
			*q = *p;
			*p = t;
			q++;
			p--;
		}
		return (TCHAR*)pStr;
	}
	static TCHAR* _cstrstr(const TCHAR* pStr, const TCHAR* pCharSet)
	{
		int nLen = lstrlen(pCharSet);
		if (nLen == 0)
			return (TCHAR*)pStr;

		const TCHAR* pRet = NULL;
		const TCHAR* pCur = pStr;
		while((pStr = _cstrchr(pCur, *pCharSet)) != NULL)
		{
			if(memcmp(pCur, pCharSet, nLen * sizeof(TCHAR)) == 0)
			{
				pRet = pCur;
				break;
			}
			pCur = ::CharNext(pCur);
		}
		return (TCHAR*) pRet;
	}
	static int _cstrspn(const TCHAR* pStr, const TCHAR* pCharSet)
	{
		int nRet = 0;
		TCHAR* p = (TCHAR*)pStr;
		while (*p != 0)
		{
			TCHAR* pNext = ::CharNext(p);
			if(pNext > p + 1)
			{
				if(_cstrchr_db(pCharSet, *p, *(p + 1)) == NULL)
					break;
				nRet += 2;
			}
			else
			{
				if(_cstrchr(pCharSet, *p) == NULL)
					break;
				nRet++;
			}
			p = pNext;
		}
		return nRet;
	}
	static int _cstrcspn(const TCHAR* pStr, const TCHAR* pCharSet)
	{
		int nRet = 0;
		TCHAR* p = (TCHAR*)pStr;
		while (*p != 0)
		{
			TCHAR* pNext = ::CharNext(p);
			if(pNext > p + 1)
			{
				if(_cstrchr_db(pCharSet, *p, *(p + 1)) != NULL)
					break;
				nRet += 2;
			}
			else
			{
				if(_cstrchr(pCharSet, *p) != NULL)
					break;
				nRet++;
			}
			p = pNext;
		}
		return nRet;
	}
	static TCHAR* _cstrpbrk(const TCHAR* p, const TCHAR* lpszCharSet)
	{
		while (*p != 0)
		{
			if (_cstrchr(lpszCharSet, *p) != NULL)
			{
				return (TCHAR*)p;
				break;
			}
			p = ::CharNext(p);
		}
		return NULL;
	}
	static int _cstrisdigit(TCHAR ch)
	{
		WORD type;
		GetStringTypeEx(GetThreadLocale(), CT_CTYPE1, &ch, 1, &type);
		return (type & C1_DIGIT) == C1_DIGIT;
	}
	static int _cstrisspace(TCHAR ch)
	{
		WORD type;
		GetStringTypeEx(GetThreadLocale(), CT_CTYPE1, &ch, 1, &type);
		return (type & C1_SPACE) == C1_SPACE;
	}
	static int _cstrcmp(const TCHAR* pstrOne, const TCHAR* pstrOther)
	{
		return lstrcmp(pstrOne, pstrOther);
	}
	static int _cstrcmpi(const TCHAR* pstrOne, const TCHAR* pstrOther)
	{
		return lstrcmpi(pstrOne, pstrOther);
	}
	static int _cstrcoll(const TCHAR* pstrOne, const TCHAR* pstrOther)
	{
		int nRet = CompareString(GetThreadLocale(), 0, pstrOne, -1, pstrOther, -1);
		ATLASSERT(nRet != 0);
		return nRet - 2;  // Convert to strcmp convention.  This really is documented.
	}
	static int _cstrcolli(const TCHAR* pstrOne, const TCHAR* pstrOther)
	{
		int nRet = CompareString(GetThreadLocale(), NORM_IGNORECASE, pstrOne, -1, pstrOther, -1);
		ATLASSERT(nRet != 0);
		return nRet - 2;  // Convert to strcmp convention.  This really is documented.
	}
	static int _cstrtoi(const TCHAR* nptr)
	{
		int c;              /* current char */
		int total;          /* current total */
		int sign;           /* if '-', then negative, otherwise positive */

		while ( _cstrisspace(*nptr) )
			++nptr;

		c = (int)(_TUCHAR)*nptr++;
		sign = c;           /* save sign indication */
		if (c == _T('-') || c == _T('+'))
			c = (int)(_TUCHAR)*nptr++;    /* skip sign */

		total = 0;

		while (_cstrisdigit((TCHAR)c)) {
			total = 10 * total + (c - '0');     /* accumulate digit */
			c = (int)(_TUCHAR)*nptr++;    /* get next char */
		}

		if (sign == '-')
			return -total;
		else
			return total;   /* return result, negated if necessary */
	}
#else //!_ATL_MIN_CRT
	static TCHAR* _cstrchr(const TCHAR* p, TCHAR ch)
	{
		return _tcschr(p, ch);
	}
	static TCHAR* _cstrrchr(const TCHAR* p, TCHAR ch)
	{
		return _tcsrchr(p, ch);
	}
	static TCHAR* _cstrrev(TCHAR* pStr)
	{
		return _tcsrev(pStr);
	}
	static TCHAR* _cstrstr(const TCHAR* pStr, const TCHAR* pCharSet)
	{
		return _tcsstr(pStr, pCharSet);
	}
	static int _cstrspn(const TCHAR* pStr, const TCHAR* pCharSet)
	{
		return (int)_tcsspn(pStr, pCharSet);
	}
	static int _cstrcspn(const TCHAR* pStr, const TCHAR* pCharSet)
	{
		return (int)_tcscspn(pStr, pCharSet);
	}
	static TCHAR* _cstrpbrk(const TCHAR* p, const TCHAR* lpszCharSet)
	{
		return _tcspbrk(p, lpszCharSet);
	}
	static int _cstrisdigit(TCHAR ch)
	{
		return _istdigit(ch);
	}
	static int _cstrisspace(TCHAR ch)
	{
		return _istspace(ch);
	}
	static int _cstrcmp(const TCHAR* pstrOne, const TCHAR* pstrOther)
	{
		return _tcscmp(pstrOne, pstrOther);
	}
	static int _cstrcmpi(const TCHAR* pstrOne, const TCHAR* pstrOther)
	{
		return _tcsicmp(pstrOne, pstrOther);
	}
	static int _cstrcoll(const TCHAR* pstrOne, const TCHAR* pstrOther)
	{
		return _tcscoll(pstrOne, pstrOther);
	}
	static int _cstrcolli(const TCHAR* pstrOne, const TCHAR* pstrOther)
	{
		return _tcsicoll(pstrOne, pstrOther);
	}
	static int _cstrtoi(const TCHAR* nptr)
	{
		return _ttoi(nptr);
	}
#endif //!_ATL_MIN_CRT
};

// Compare helpers
bool __stdcall operator==(const CString& s1, const CString& s2);
bool __stdcall operator==(const CString& s1, LPCTSTR s2);
bool __stdcall operator==(LPCTSTR s1, const CString& s2);
bool __stdcall operator!=(const CString& s1, const CString& s2);
bool __stdcall operator!=(const CString& s1, LPCTSTR s2);
bool __stdcall operator!=(LPCTSTR s1, const CString& s2);
bool __stdcall operator<(const CString& s1, const CString& s2);
bool __stdcall operator<(const CString& s1, LPCTSTR s2);
bool __stdcall operator<(LPCTSTR s1, const CString& s2);
bool __stdcall operator>(const CString& s1, const CString& s2);
bool __stdcall operator>(const CString& s1, LPCTSTR s2);
bool __stdcall operator>(LPCTSTR s1, const CString& s2);
bool __stdcall operator<=(const CString& s1, const CString& s2);
bool __stdcall operator<=(const CString& s1, LPCTSTR s2);
bool __stdcall operator<=(LPCTSTR s1, const CString& s2);
bool __stdcall operator>=(const CString& s1, const CString& s2);
bool __stdcall operator>=(const CString& s1, LPCTSTR s2);
bool __stdcall operator>=(LPCTSTR s1, const CString& s2);


/////////////////////////////////////////////////////////////////////////////
// CString Implementation

inline CStringData* CString::GetData() const
	{ ATLASSERT(m_pchData != NULL); return ((CStringData*)m_pchData) - 1; }
inline void CString::Init()
	{ m_pchData = _GetEmptyString().m_pchData; }
inline CString::CString(const unsigned char* lpsz)
	{ Init(); *this = (LPCSTR)lpsz; }
inline const CString& CString::operator=(const unsigned char* lpsz)
	{ *this = (LPCSTR)lpsz; return *this; }
#ifdef _UNICODE
inline const CString& CString::operator+=(char ch)
	{ *this += (TCHAR)ch; return *this; }
inline const CString& CString::operator=(char ch)
	{ *this = (TCHAR)ch; return *this; }
inline CString __stdcall operator+(const CString& string, char ch)
	{ return string + (TCHAR)ch; }
inline CString __stdcall operator+(char ch, const CString& string)
	{ return (TCHAR)ch + string; }
#endif

inline int CString::GetLength() const
	{ return GetData()->nDataLength; }
inline int CString::GetAllocLength() const
	{ return GetData()->nAllocLength; }
inline BOOL CString::IsEmpty() const
	{ return GetData()->nDataLength == 0; }
inline CString::operator LPCTSTR() const
	{ return m_pchData; }
inline int PASCAL CString::SafeStrlen(LPCTSTR lpsz)
	{ return (lpsz == NULL) ? 0 : lstrlen(lpsz); }

// CString support (windows specific)
inline int CString::Compare(LPCTSTR lpsz) const
	{ return _cstrcmp(m_pchData, lpsz); }    // MBCS/Unicode aware
inline int CString::CompareNoCase(LPCTSTR lpsz) const
	{ return _cstrcmpi(m_pchData, lpsz); }   // MBCS/Unicode aware
// CString::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
inline int CString::Collate(LPCTSTR lpsz) const
	{ return _cstrcoll(m_pchData, lpsz); }   // locale sensitive
inline int CString::CollateNoCase(LPCTSTR lpsz) const
	{ return _cstrcolli(m_pchData, lpsz); }   // locale sensitive

inline TCHAR CString::GetAt(int nIndex) const
{
	ATLASSERT(nIndex >= 0);
	ATLASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
inline TCHAR CString::operator[](int nIndex) const
{
	// same as GetAt
	ATLASSERT(nIndex >= 0);
	ATLASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
inline bool __stdcall operator==(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) == 0; }
inline bool __stdcall operator==(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) == 0; }
inline bool __stdcall operator==(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) == 0; }
inline bool __stdcall operator!=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) != 0; }
inline bool __stdcall operator!=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) != 0; }
inline bool __stdcall operator!=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) != 0; }
inline bool __stdcall operator<(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) < 0; }
inline bool __stdcall operator<(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) < 0; }
inline bool __stdcall operator<(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) > 0; }
inline bool __stdcall operator>(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) > 0; }
inline bool __stdcall operator>(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) > 0; }
inline bool __stdcall operator>(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) < 0; }
inline bool __stdcall operator<=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) <= 0; }
inline bool __stdcall operator<=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) <= 0; }
inline bool __stdcall operator<=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) >= 0; }
inline bool __stdcall operator>=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) >= 0; }
inline bool __stdcall operator>=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) >= 0; }
inline bool __stdcall operator>=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) <= 0; }

inline CString::CString()
{
	Init();
}

inline CString::CString(const CString& stringSrc)
{
	ATLASSERT(stringSrc.GetData()->nRefs != 0);
	if (stringSrc.GetData()->nRefs >= 0)
	{
		ATLASSERT(stringSrc.GetData() != _atltmpDataNil);
		m_pchData = stringSrc.m_pchData;
		InterlockedIncrement(&GetData()->nRefs);
	}
	else
	{
		Init();
		*this = stringSrc.m_pchData;
	}
}

inline BOOL CString::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
	ATLASSERT(nLen >= 0);
	ATLASSERT(nLen <= INT_MAX - 1);    // max size (enough room for 1 extra)

	if (nLen == 0)
	{
		Init();
	}
	else
	{
		CStringData* pData = NULL;
		ATLTRY(pData = (CStringData*)new BYTE[sizeof(CStringData) + (nLen + 1) * sizeof(TCHAR)]);
		if(pData == NULL)
			return FALSE;

		pData->nRefs = 1;
		pData->data()[nLen] = '\0';
		pData->nDataLength = nLen;
		pData->nAllocLength = nLen;
		m_pchData = pData->data();
	}

	return TRUE;
}

inline void CString::Release()
{
	if (GetData() != _atltmpDataNil)
	{
		ATLASSERT(GetData()->nRefs != 0);
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			delete[] (BYTE*)GetData();
		Init();
	}
}

inline void PASCAL CString::Release(CStringData* pData)
{
	if (pData != _atltmpDataNil)
	{
		ATLASSERT(pData->nRefs != 0);
		if (InterlockedDecrement(&pData->nRefs) <= 0)
			delete[] (BYTE*)pData;
	}
}

inline void CString::Empty()
{
	if (GetData()->nDataLength == 0)
		return;

	if (GetData()->nRefs >= 0)
		Release();
	else
		*this = _T("");

	ATLASSERT(GetData()->nDataLength == 0);
	ATLASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

inline void CString::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		CStringData* pData = GetData();
		Release();
		if(AllocBuffer(pData->nDataLength))
			memcpy(m_pchData, pData->data(), (pData->nDataLength + 1) * sizeof(TCHAR));
	}
	ATLASSERT(GetData()->nRefs <= 1);
}

inline BOOL CString::AllocBeforeWrite(int nLen)
{
	BOOL bRet = TRUE;
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		bRet = AllocBuffer(nLen);
	}
	ATLASSERT(GetData()->nRefs <= 1);
	return bRet;
}

inline CString::~CString()
//  free any attached data
{
	if (GetData() != _atltmpDataNil)
	{
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			delete[] (BYTE*)GetData();
	}
}

inline void CString::AllocCopy(CString& dest, int nCopyLen, int nCopyIndex,
	 int nExtraLen) const
{
	// will clone the data attached to this string
	// allocating 'nExtraLen' characters
	// Places results in uninitialized string 'dest'
	// Will copy the part or all of original data to start of new string

	int nNewLen = nCopyLen + nExtraLen;
	if (nNewLen == 0)
	{
		dest.Init();
	}
	else
	{
		if(dest.AllocBuffer(nNewLen))
			memcpy(dest.m_pchData, m_pchData + nCopyIndex, nCopyLen * sizeof(TCHAR));
	}
}

inline CString::CString(LPCTSTR lpsz)
{
	Init();
	if (lpsz != NULL && HIWORD(lpsz) == NULL)
	{
		UINT nID = LOWORD((DWORD_PTR)lpsz);
		if (!LoadString(nID))
			ATLTRACE2(atlTraceUI, 0, _T("Warning: implicit LoadString(%u) in CString failed\n"), nID);
	}
	else
	{
		int nLen = SafeStrlen(lpsz);
		if (nLen != 0)
		{
			if(AllocBuffer(nLen))
				memcpy(m_pchData, lpsz, nLen * sizeof(TCHAR));
		}
	}
}

#ifdef _UNICODE
inline CString::CString(LPCSTR lpsz)
{
	Init();
	int nSrcLen = (lpsz != NULL) ? lstrlenA(lpsz) : 0;
	if (nSrcLen != 0)
	{
		if(AllocBuffer(nSrcLen))
		{
			_mbstowcsz(m_pchData, lpsz, nSrcLen + 1);
			ReleaseBuffer();
		}
	}
}
#else //_UNICODE
inline CString::CString(LPCWSTR lpsz)
{
	Init();
	int nSrcLen = (lpsz != NULL) ? (int)wcslen(lpsz) : 0;
	if (nSrcLen != 0)
	{
		if(AllocBuffer(nSrcLen * 2))
		{
			_wcstombsz(m_pchData, lpsz, (nSrcLen * 2) + 1);
			ReleaseBuffer();
		}
	}
}
#endif //!_UNICODE

// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

inline void CString::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
	if(AllocBeforeWrite(nSrcLen))
	{
		memcpy(m_pchData, lpszSrcData, nSrcLen * sizeof(TCHAR));
		GetData()->nDataLength = nSrcLen;
		m_pchData[nSrcLen] = '\0';
	}
}

inline const CString& CString::operator=(const CString& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != _atltmpDataNil) || stringSrc.GetData()->nRefs < 0)
		{
			// actual copy necessary since one of the strings is locked
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			// can just copy references around
			Release();
			ATLASSERT(stringSrc.GetData() != _atltmpDataNil);
			m_pchData = stringSrc.m_pchData;
			InterlockedIncrement(&GetData()->nRefs);
		}
	}
	return *this;
}

inline const CString& CString::operator=(LPCTSTR lpsz)
{
	ATLASSERT(lpsz == NULL || _IsValidString(lpsz));
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

#ifdef _UNICODE
inline const CString& CString::operator=(LPCSTR lpsz)
{
	int nSrcLen = (lpsz != NULL) ? lstrlenA(lpsz) : 0;
	if(AllocBeforeWrite(nSrcLen))
	{
		_mbstowcsz(m_pchData, lpsz, nSrcLen + 1);
		ReleaseBuffer();
	}
	return *this;
}
#else //!_UNICODE
inline const CString& CString::operator=(LPCWSTR lpsz)
{
	int nSrcLen = (lpsz != NULL) ? (int)wcslen(lpsz) : 0;
	if(AllocBeforeWrite(nSrcLen * 2))
	{
		_wcstombsz(m_pchData, lpsz, (nSrcLen * 2) + 1);
		ReleaseBuffer();
	}
	return *this;
}
#endif  //!_UNICODE

// Concatenation
// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CString + CString
// and for ? = TCHAR, LPCTSTR
//          CString + ?
//          ? + CString

inline BOOL CString::ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data,
	int nSrc2Len, LPCTSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CString object

	BOOL bRet = TRUE;
	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		bRet = AllocBuffer(nNewLen);
		if (bRet)
		{
			memcpy(m_pchData, lpszSrc1Data, nSrc1Len * sizeof(TCHAR));
			memcpy(m_pchData + nSrc1Len, lpszSrc2Data, nSrc2Len * sizeof(TCHAR));
		}
	}
	return bRet;
}

inline CString __stdcall operator+(const CString& string1, const CString& string2)
{
	CString s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

inline CString __stdcall operator+(const CString& string, LPCTSTR lpsz)
{
	ATLASSERT(lpsz == NULL || CString::_IsValidString(lpsz));
	CString s;
	s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData, CString::SafeStrlen(lpsz), lpsz);
	return s;
}

inline CString __stdcall operator+(LPCTSTR lpsz, const CString& string)
{
	ATLASSERT(lpsz == NULL || CString::_IsValidString(lpsz));
	CString s;
	s.ConcatCopy(CString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength, string.m_pchData);
	return s;
}

inline void CString::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
	//  -- the main routine for += operators

	// concatenating an empty string is a no-op!
	if (nSrcLen == 0)
		return;

	// if the buffer is too small, or we have a width mis-match, just
	//   allocate a new buffer (slow but sure)
	if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
	{
		// we have to grow the buffer, use the ConcatCopy routine
		CStringData* pOldData = GetData();
		if (ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData))
		{
			ATLASSERT(pOldData != NULL);
			CString::Release(pOldData);
		}
	}
	else
	{
		// fast concatenation when buffer big enough
		memcpy(m_pchData + GetData()->nDataLength, lpszSrcData, nSrcLen * sizeof(TCHAR));
		GetData()->nDataLength += nSrcLen;
		ATLASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
		m_pchData[GetData()->nDataLength] = '\0';
	}
}

inline const CString& CString::operator+=(LPCTSTR lpsz)
{
	ATLASSERT(lpsz == NULL || _IsValidString(lpsz));
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

inline const CString& CString::operator+=(TCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

inline const CString& CString::operator+=(const CString& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

inline LPTSTR CString::GetBuffer(int nMinBufLength)
{
	ATLASSERT(nMinBufLength >= 0);

	if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
	{
		// we have to grow the buffer
		CStringData* pOldData = GetData();
		int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
		if (nMinBufLength < nOldLen)
			nMinBufLength = nOldLen;

		if(!AllocBuffer(nMinBufLength))
			return NULL;

		memcpy(m_pchData, pOldData->data(), (nOldLen + 1) * sizeof(TCHAR));
		GetData()->nDataLength = nOldLen;
		CString::Release(pOldData);
	}
	ATLASSERT(GetData()->nRefs <= 1);

	// return a pointer to the character storage for this string
	ATLASSERT(m_pchData != NULL);
	return m_pchData;
}

inline void CString::ReleaseBuffer(int nNewLength)
{
	CopyBeforeWrite();  // just in case GetBuffer was not called

	if (nNewLength == -1)
		nNewLength = lstrlen(m_pchData); // zero terminated

	ATLASSERT(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

inline LPTSTR CString::GetBufferSetLength(int nNewLength)
{
	ATLASSERT(nNewLength >= 0);

	if(GetBuffer(nNewLength) == NULL)
		return NULL;

	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
	return m_pchData;
}

inline void CString::FreeExtra()
{
	ATLASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
	if (GetData()->nDataLength != GetData()->nAllocLength)
	{
		CStringData* pOldData = GetData();
		if(AllocBuffer(GetData()->nDataLength))
		{
			memcpy(m_pchData, pOldData->data(), pOldData->nDataLength * sizeof(TCHAR));
			ATLASSERT(m_pchData[GetData()->nDataLength] == '\0');
			CString::Release(pOldData);
		}
	}
	ATLASSERT(GetData() != NULL);
}

inline LPTSTR CString::LockBuffer()
{
	LPTSTR lpsz = GetBuffer(0);
	if(lpsz != NULL)
		GetData()->nRefs = -1;
	return lpsz;
}

inline void CString::UnlockBuffer()
{
	ATLASSERT(GetData()->nRefs == -1);
	if (GetData() != _atltmpDataNil)
		GetData()->nRefs = 1;
}

inline int CString::Find(TCHAR ch) const
{
	return Find(ch, 0);
}

inline int CString::Find(TCHAR ch, int nStart) const
{
	int nLength = GetData()->nDataLength;
	if (nStart >= nLength)
		return -1;

	// find first single character
	LPTSTR lpsz = _cstrchr(m_pchData + nStart, (_TUCHAR)ch);

	// return -1 if not found and index otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

inline int CString::FindOneOf(LPCTSTR lpszCharSet) const
{
	ATLASSERT(_IsValidString(lpszCharSet));
	LPTSTR lpsz = _cstrpbrk(m_pchData, lpszCharSet);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

inline void CString::MakeUpper()
{
	CopyBeforeWrite();
	CharUpper(m_pchData);
}

inline void CString::MakeLower()
{
	CopyBeforeWrite();
	CharLower(m_pchData);
}

inline void CString::MakeReverse()
{
	CopyBeforeWrite();
	_cstrrev(m_pchData);
}

inline void CString::SetAt(int nIndex, TCHAR ch)
{
	ATLASSERT(nIndex >= 0);
	ATLASSERT(nIndex < GetData()->nDataLength);

	CopyBeforeWrite();
	m_pchData[nIndex] = ch;
}

#ifndef _UNICODE
inline void CString::AnsiToOem()
{
	CopyBeforeWrite();
	::AnsiToOem(m_pchData, m_pchData);
}
inline void CString::OemToAnsi()
{
	CopyBeforeWrite();
	::OemToAnsi(m_pchData, m_pchData);
}
#endif

inline CString::CString(TCHAR ch, int nLength)
{
	ATLASSERT(!_istlead(ch));    // can't create a lead byte string
	Init();
	if (nLength >= 1)
	{
		if(AllocBuffer(nLength))
		{
#ifdef _UNICODE
			for (int i = 0; i < nLength; i++)
				m_pchData[i] = ch;
#else
			memset(m_pchData, ch, nLength);
#endif
		}
	}
}

inline CString::CString(LPCTSTR lpch, int nLength)
{
	Init();
	if (nLength != 0)
	{
		if(AllocBuffer(nLength))
			memcpy(m_pchData, lpch, nLength * sizeof(TCHAR));
	}
}

#ifdef _UNICODE
inline CString::CString(LPCSTR lpsz, int nLength)
{
	Init();
	if (nLength != 0)
	{
		if(AllocBuffer(nLength))
		{
			int n = ::MultiByteToWideChar(CP_ACP, 0, lpsz, nLength, m_pchData, nLength + 1);
			ReleaseBuffer((n >= 0) ? n : -1);
		}
	}
}
#else //_UNICODE
inline CString::CString(LPCWSTR lpsz, int nLength)
{
	Init();
	if (nLength != 0)
	{
		if(AllocBuffer(nLength * 2))
		{
			int n = ::WideCharToMultiByte(CP_ACP, 0, lpsz, nLength, m_pchData, (nLength * 2) + 1, NULL, NULL);
			ReleaseBuffer((n >= 0) ? n : -1);
		}
	}
}
#endif //!_UNICODE

inline const CString& CString::operator=(TCHAR ch)
{
	ATLASSERT(!_istlead(ch));    // can't set single lead byte
	AssignCopy(1, &ch);
	return *this;
}

inline CString __stdcall operator+(const CString& string1, TCHAR ch)
{
	CString s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
	return s;
}

inline CString __stdcall operator+(TCHAR ch, const CString& string)
{
	CString s;
	s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
	return s;
}

inline CString CString::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

inline CString CString::Mid(int nFirst, int nCount) const
{
	// out-of-bounds requests return sensible things
	if (nFirst < 0)
		nFirst = 0;
	if (nCount < 0)
		nCount = 0;

	if (nFirst + nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength - nFirst;
	if (nFirst > GetData()->nDataLength)
		nCount = 0;

	CString dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

inline CString CString::Right(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	else if (nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength;

	CString dest;
	AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
	return dest;
}

inline CString CString::Left(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	else if (nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength;

	CString dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

// strspn equivalent
inline CString CString::SpanIncluding(LPCTSTR lpszCharSet) const
{
	ATLASSERT(_IsValidString(lpszCharSet));
	return Left(_cstrspn(m_pchData, lpszCharSet));
}

// strcspn equivalent
inline CString CString::SpanExcluding(LPCTSTR lpszCharSet) const
{
	ATLASSERT(_IsValidString(lpszCharSet));
	return Left(_cstrcspn(m_pchData, lpszCharSet));
}

inline int CString::ReverseFind(TCHAR ch) const
{
	// find last single character
	LPTSTR lpsz = _cstrrchr(m_pchData, (_TUCHAR)ch);

	// return -1 if not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
inline int CString::Find(LPCTSTR lpszSub) const
{
	return Find(lpszSub, 0);
}

inline int CString::Find(LPCTSTR lpszSub, int nStart) const
{
	ATLASSERT(_IsValidString(lpszSub));

	int nLength = GetData()->nDataLength;
	if (nStart > nLength)
		return -1;

	// find first matching substring
	LPTSTR lpsz = _cstrstr(m_pchData + nStart, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

inline BOOL CString::FormatV(LPCTSTR lpszFormat, va_list argList)
{
	ATLASSERT(_IsValidString(lpszFormat));

	enum _FormatModifiers
	{
		FORCE_ANSI =	0x10000,
		FORCE_UNICODE =	0x20000,
		FORCE_INT64 =	0x40000
	};

	va_list argListSave = argList;

	// make a guess at the maximum length of the resulting string
	int nMaxLen = 0;
	for (LPCTSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = ::CharNext(lpsz))
	{
		// handle '%' character, but watch out for '%%'
		if (*lpsz != '%' || *(lpsz = ::CharNext(lpsz)) == '%')
		{
			// this is instead of _tclen()
#if !defined(_UNICODE) && defined(_MBCS)
			nMaxLen += (int)(::CharNext(lpsz) - lpsz);
#else
			nMaxLen++;
#endif
			continue;
		}

		int nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *lpsz != '\0'; lpsz = ::CharNext(lpsz))
		{
			// check for valid flags
			if (*lpsz == '#')
				nMaxLen += 2;   // for '0x'
			else if (*lpsz == '*')
				nWidth = va_arg(argList, int);
			else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' || *lpsz == ' ')
				;
			else // hit non-flag character
				break;
		}
		// get width and skip it
		if (nWidth == 0)
		{
			// width indicated by
			nWidth = _cstrtoi(lpsz);
			for (; *lpsz != '\0' && _cstrisdigit(*lpsz); lpsz = ::CharNext(lpsz))
				;
		}
		ATLASSERT(nWidth >= 0);

		int nPrecision = 0;
		if (*lpsz == '.')
		{
			// skip past '.' separator (width.precision)
			lpsz = ::CharNext(lpsz);

			// get precision and skip it
			if (*lpsz == '*')
			{
				nPrecision = va_arg(argList, int);
				lpsz = ::CharNext(lpsz);
			}
			else
			{
				nPrecision = _cstrtoi(lpsz);
				for (; *lpsz != '\0' && _cstrisdigit(*lpsz); lpsz = ::CharNext(lpsz))
					;
			}
			ATLASSERT(nPrecision >= 0);
		}

		// should be on type modifier or specifier
		int nModifier = 0;
		if(lpsz[0] == _T('I') && lpsz[1] == _T('6') && lpsz[2] == _T('4'))
		{
			lpsz += 3;
			nModifier = FORCE_INT64;
		}
		else
		{
			switch (*lpsz)
			{
			// modifiers that affect size
			case 'h':
				nModifier = FORCE_ANSI;
				lpsz = ::CharNext(lpsz);
				break;
			case 'l':
				nModifier = FORCE_UNICODE;
				lpsz = ::CharNext(lpsz);
				break;

			// modifiers that do not affect size
			case 'F':
			case 'N':
			case 'L':
				lpsz = ::CharNext(lpsz);
				break;
			}
		}

		// now should be on specifier
		switch (*lpsz | nModifier)
		{
		// single characters
		case 'c':
		case 'C':
			nItemLen = 2;
			va_arg(argList, TCHAR);
			break;
		case 'c' | FORCE_ANSI:
		case 'C' | FORCE_ANSI:
			nItemLen = 2;
			va_arg(argList, char);
			break;
		case 'c' | FORCE_UNICODE:
		case 'C' | FORCE_UNICODE:
			nItemLen = 2;
			va_arg(argList, WCHAR);
			break;

		// strings
		case 's':
		{
			LPCTSTR pstrNextArg = va_arg(argList, LPCTSTR);
			if (pstrNextArg == NULL)
			{
				nItemLen = 6;  // "(null)"
			}
			else
			{
				nItemLen = lstrlen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
			break;
		}

		case 'S':
		{
#ifndef _UNICODE
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
			{
				nItemLen = 6;  // "(null)"
			}
			else
			{
				nItemLen = (int)wcslen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
#else
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL)
			{
				nItemLen = 6; // "(null)"
			}
			else
			{
				nItemLen = lstrlenA(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
#endif
			break;
		}

		case 's' | FORCE_ANSI:
		case 'S' | FORCE_ANSI:
		{
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL)
			{
				nItemLen = 6; // "(null)"
			}
			else
			{
				nItemLen = lstrlenA(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
			break;
		}

		case 's' | FORCE_UNICODE:
		case 'S' | FORCE_UNICODE:
		{
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
			{
				nItemLen = 6; // "(null)"
			}
			else
			{
				nItemLen = (int)wcslen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
			break;
		}
		}

		// adjust nItemLen for strings
		if (nItemLen != 0)
		{
			nItemLen = max(nItemLen, nWidth);
			if (nPrecision != 0)
				nItemLen = min(nItemLen, nPrecision);
		}
		else
		{
			switch (*lpsz)
			{
			// integers
			case 'd':
			case 'i':
			case 'u':
			case 'x':
			case 'X':
			case 'o':
				if (nModifier & FORCE_INT64)
					va_arg(argList, __int64);
				else
					va_arg(argList, int);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth + nPrecision);
				break;

#ifndef _ATL_USE_CSTRING_FLOAT
			case 'e':
			case 'f':
			case 'g':
			case 'G':
				ATLASSERT(!"Floating point (%%e, %%f, %%g, and %%G) is not supported by the WTL::CString class.");
#ifndef _DEBUG
				::OutputDebugString(_T("Floating point (%%e, %%f, %%g, and %%G) is not supported by the WTL::CString class."));
				::DebugBreak();
#endif //!_DEBUG
				break;
#else //_ATL_USE_CSTRING_FLOAT
			case 'e':
			case 'g':
			case 'G':
				va_arg(argList, double);
				nItemLen = 128;
				nItemLen = max(nItemLen, nWidth + nPrecision);
				break;
			case 'f':
				{
					double f;
					LPTSTR pszTemp;

					// 312 == strlen("-1+(309 zeroes).")
					// 309 zeroes == max precision of a double
					// 6 == adjustment in case precision is not specified,
					//   which means that the precision defaults to 6
					pszTemp = (LPTSTR)_alloca(max(nWidth, 312 + nPrecision + 6));

					f = va_arg(argList, double);
					_stprintf(pszTemp, _T( "%*.*f" ), nWidth, nPrecision + 6, f);
					nItemLen = _tcslen(pszTemp);
				}
				break;
#endif //_ATL_USE_CSTRING_FLOAT

			case 'p':
				va_arg(argList, void*);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth + nPrecision);
				break;

			// no output
			case 'n':
				va_arg(argList, int*);
				break;

			default:
				ATLASSERT(FALSE);  // unknown formatting option
			}
		}

		// adjust nMaxLen for output nItemLen
		nMaxLen += nItemLen;
	}

	if(GetBuffer(nMaxLen) == NULL)
		return FALSE;
#ifndef _ATL_USE_CSTRING_FLOAT
	int nRet = wvsprintf(m_pchData, lpszFormat, argListSave);
#else //_ATL_USE_CSTRING_FLOAT
	int nRet = _vstprintf(m_pchData, lpszFormat, argListSave);
#endif //_ATL_USE_CSTRING_FLOAT
	nRet;	// ref
	ATLASSERT(nRet <= GetAllocLength());
	ReleaseBuffer();

	va_end(argListSave);
	return TRUE;
}

// formatting (using wsprintf style formatting)
inline BOOL __cdecl CString::Format(LPCTSTR lpszFormat, ...)
{
	ATLASSERT(_IsValidString(lpszFormat));

	va_list argList;
	va_start(argList, lpszFormat);
	BOOL bRet = FormatV(lpszFormat, argList);
	va_end(argList);
	return bRet;
}

inline BOOL __cdecl CString::Format(UINT nFormatID, ...)
{
	CString strFormat;
	BOOL bRet = strFormat.LoadString(nFormatID);
	ATLASSERT(bRet != 0);

	va_list argList;
	va_start(argList, nFormatID);
	bRet = FormatV(strFormat, argList);
	va_end(argList);
	return bRet;
}

// formatting (using FormatMessage style formatting)
inline BOOL __cdecl CString::FormatMessage(LPCTSTR lpszFormat, ...)
{
	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, lpszFormat);
	LPTSTR lpszTemp;
	BOOL bRet = TRUE;

	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			lpszFormat, 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 || lpszTemp == NULL)
		bRet = FALSE;

	// assign lpszTemp into the resulting string and free the temporary
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
	return bRet;
}

inline BOOL __cdecl CString::FormatMessage(UINT nFormatID, ...)
{
	// get format string from string table
	CString strFormat;
	BOOL bRetTmp = strFormat.LoadString(nFormatID);
	bRetTmp;	// ref
	ATLASSERT(bRetTmp != 0);

	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, nFormatID);
	LPTSTR lpszTemp;
	BOOL bRet = TRUE;

	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			strFormat, 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 || lpszTemp == NULL)
		bRet = FALSE;

	// assign lpszTemp into the resulting string and free lpszTemp
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
	return bRet;
}

inline void CString::TrimRight()
{
	CopyBeforeWrite();

	// find beginning of trailing spaces by starting at beginning (DBCS aware)
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;
	while (*lpsz != '\0')
	{
		if (_cstrisspace(*lpsz))
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
		{
			lpszLast = NULL;
		}
		lpsz = ::CharNext(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at trailing space start
		*lpszLast = '\0';
		GetData()->nDataLength = (int)(DWORD_PTR)(lpszLast - m_pchData);
	}
}

inline void CString::TrimLeft()
{
	CopyBeforeWrite();

	// find first non-space character
	LPCTSTR lpsz = m_pchData;
	while (_cstrisspace(*lpsz))
		lpsz = ::CharNext(lpsz);

	// fix up data and length
	int nDataLength = GetData()->nDataLength - (int)(DWORD_PTR)(lpsz - m_pchData);
	memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(TCHAR));
	GetData()->nDataLength = nDataLength;
}

inline void CString::TrimRight(LPCTSTR lpszTargetList)
{
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (_cstrchr(lpszTargetList, *lpsz) != NULL)
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = ::CharNext(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at left-most matching character
		*lpszLast = '\0';
		GetData()->nDataLength = (int)(DWORD_PTR)(lpszLast - m_pchData);
	}
}

inline void CString::TrimRight(TCHAR chTarget)
{
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (*lpsz == chTarget)
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = ::CharNext(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at left-most matching character
		*lpszLast = '\0';
		GetData()->nDataLength = (int)(DWORD_PTR)(lpszLast - m_pchData);
	}
}

inline void CString::TrimLeft(LPCTSTR lpszTargets)
{
	// if we're not trimming anything, we're not doing any work
	if (SafeStrlen(lpszTargets) == 0)
		return;

	CopyBeforeWrite();
	LPCTSTR lpsz = m_pchData;

	while (*lpsz != '\0')
	{
		if (_cstrchr(lpszTargets, *lpsz) == NULL)
			break;
		lpsz = ::CharNext(lpsz);
	}

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (int)(DWORD_PTR)(lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(TCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

inline void CString::TrimLeft(TCHAR chTarget)
{
	// find first non-matching character

	CopyBeforeWrite();
	LPCTSTR lpsz = m_pchData;

	while (chTarget == *lpsz)
		lpsz = ::CharNext(lpsz);

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (int)(DWORD_PTR)(lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength + 1) * sizeof(TCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

inline int CString::Delete(int nIndex, int nCount /* = 1 */)
{
	if (nIndex < 0)
		nIndex = 0;
	int nNewLength = GetData()->nDataLength;
	if (nCount > 0 && nIndex < nNewLength)
	{
		CopyBeforeWrite();
		int nBytesToCopy = nNewLength - (nIndex + nCount) + 1;

		memmove(m_pchData + nIndex, m_pchData + nIndex + nCount, nBytesToCopy * sizeof(TCHAR));
		GetData()->nDataLength = nNewLength - nCount;
	}

	return nNewLength;
}

inline int CString::Insert(int nIndex, TCHAR ch)
{
	CopyBeforeWrite();

	if (nIndex < 0)
		nIndex = 0;

	int nNewLength = GetData()->nDataLength;
	if (nIndex > nNewLength)
		nIndex = nNewLength;
	nNewLength++;

	if (GetData()->nAllocLength < nNewLength)
	{
		CStringData* pOldData = GetData();
		LPTSTR pstr = m_pchData;
		if(!AllocBuffer(nNewLength))
			return -1;
		memcpy(m_pchData, pstr, (pOldData->nDataLength + 1) * sizeof(TCHAR));
		CString::Release(pOldData);
	}

	// move existing bytes down
	memmove(m_pchData + nIndex + 1, m_pchData + nIndex, (nNewLength - nIndex) * sizeof(TCHAR));
	m_pchData[nIndex] = ch;
	GetData()->nDataLength = nNewLength;

	return nNewLength;
}

inline int CString::Insert(int nIndex, LPCTSTR pstr)
{
	if (nIndex < 0)
		nIndex = 0;

	int nInsertLength = SafeStrlen(pstr);
	int nNewLength = GetData()->nDataLength;
	if (nInsertLength > 0)
	{
		CopyBeforeWrite();
		if (nIndex > nNewLength)
			nIndex = nNewLength;
		nNewLength += nInsertLength;

		if (GetData()->nAllocLength < nNewLength)
		{
			CStringData* pOldData = GetData();
			LPTSTR pstr = m_pchData;
			if(!AllocBuffer(nNewLength))
				return -1;
			memcpy(m_pchData, pstr, (pOldData->nDataLength + 1) * sizeof(TCHAR));
			CString::Release(pOldData);
		}

		// move existing bytes down
		memmove(m_pchData + nIndex + nInsertLength, m_pchData + nIndex, (nNewLength - nIndex - nInsertLength + 1) * sizeof(TCHAR));
		memcpy(m_pchData + nIndex, pstr, nInsertLength * sizeof(TCHAR));
		GetData()->nDataLength = nNewLength;
	}

	return nNewLength;
}

inline int CString::Replace(TCHAR chOld, TCHAR chNew)
{
	int nCount = 0;

	// short-circuit the nop case
	if (chOld != chNew)
	{
		// otherwise modify each character that matches in the string
		CopyBeforeWrite();
		LPTSTR psz = m_pchData;
		LPTSTR pszEnd = psz + GetData()->nDataLength;
		while (psz < pszEnd)
		{
			// replace instances of the specified character only
			if (*psz == chOld)
			{
				*psz = chNew;
				nCount++;
			}
			psz = ::CharNext(psz);
		}
	}
	return nCount;
}

inline int CString::Replace(LPCTSTR lpszOld, LPCTSTR lpszNew)
{
	// can't have empty or NULL lpszOld

	int nSourceLen = SafeStrlen(lpszOld);
	if (nSourceLen == 0)
		return 0;
	int nReplacementLen = SafeStrlen(lpszNew);

	// loop once to figure out the size of the result string
	int nCount = 0;
	LPTSTR lpszStart = m_pchData;
	LPTSTR lpszEnd = m_pchData + GetData()->nDataLength;
	LPTSTR lpszTarget;
	while (lpszStart < lpszEnd)
	{
		while ((lpszTarget = _cstrstr(lpszStart, lpszOld)) != NULL)
		{
			nCount++;
			lpszStart = lpszTarget + nSourceLen;
		}
		lpszStart += lstrlen(lpszStart) + 1;
	}

	// if any changes were made, make them
	if (nCount > 0)
	{
		CopyBeforeWrite();

		// if the buffer is too small, just
		//   allocate a new buffer (slow but sure)
		int nOldLength = GetData()->nDataLength;
		int nNewLength =  nOldLength + (nReplacementLen - nSourceLen) * nCount;
		if (GetData()->nAllocLength < nNewLength || GetData()->nRefs > 1)
		{
			CStringData* pOldData = GetData();
			LPTSTR pstr = m_pchData;
			if(!AllocBuffer(nNewLength))
				return -1;
			memcpy(m_pchData, pstr, pOldData->nDataLength * sizeof(TCHAR));
			CString::Release(pOldData);
		}
		// else, we just do it in-place
		lpszStart = m_pchData;
		lpszEnd = m_pchData + GetData()->nDataLength;

		// loop again to actually do the work
		while (lpszStart < lpszEnd)
		{
			while ( (lpszTarget = _cstrstr(lpszStart, lpszOld)) != NULL)
			{
				int nBalance = nOldLength - ((int)(DWORD_PTR)(lpszTarget - m_pchData) + nSourceLen);
				memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen, nBalance * sizeof(TCHAR));
				memcpy(lpszTarget, lpszNew, nReplacementLen * sizeof(TCHAR));
				lpszStart = lpszTarget + nReplacementLen;
				lpszStart[nBalance] = '\0';
				nOldLength += (nReplacementLen - nSourceLen);
			}
			lpszStart += lstrlen(lpszStart) + 1;
		}
		ATLASSERT(m_pchData[nNewLength] == '\0');
		GetData()->nDataLength = nNewLength;
	}

	return nCount;
}

inline int CString::Remove(TCHAR chRemove)
{
	CopyBeforeWrite();

	LPTSTR pstrSource = m_pchData;
	LPTSTR pstrDest = m_pchData;
	LPTSTR pstrEnd = m_pchData + GetData()->nDataLength;

	while (pstrSource < pstrEnd)
	{
		if (*pstrSource != chRemove)
		{
			*pstrDest = *pstrSource;
			pstrDest = ::CharNext(pstrDest);
		}
		pstrSource = ::CharNext(pstrSource);
	}
	*pstrDest = '\0';
	int nCount = (int)(DWORD_PTR)(pstrSource - pstrDest);
	GetData()->nDataLength -= nCount;

	return nCount;
}

#ifdef _UNICODE
#define CHAR_FUDGE 1    // one TCHAR unused is good enough
#else
#define CHAR_FUDGE 2    // two BYTES unused for case of DBC last char
#endif

inline BOOL CString::LoadString(UINT nID)
{
	// try fixed buffer first (to avoid wasting space in the heap)
	TCHAR szTemp[256];
	int nCount =  sizeof(szTemp) / sizeof(szTemp[0]);
	int nLen = _LoadString(nID, szTemp, nCount);
	if (nCount - nLen > CHAR_FUDGE)
	{
		*this = szTemp;
		return (nLen > 0);
	}

	// try buffer size of 512, then larger size until entire string is retrieved
	int nSize = 256;
	do
	{
		nSize += 256;
		LPTSTR lpstr = GetBuffer(nSize - 1);
		if(lpstr == NULL)
		{
			nLen = 0;
			break;
		}
		nLen = _LoadString(nID, lpstr, nSize);
	} while (nSize - nLen <= CHAR_FUDGE);
	ReleaseBuffer();

	return (nLen > 0);
}

#ifndef _ATL_NO_COM
inline BSTR CString::AllocSysString() const
{
#if defined(_UNICODE) || defined(OLE2ANSI)
	BSTR bstr = ::SysAllocStringLen(m_pchData, GetData()->nDataLength);
#else
	int nLen = MultiByteToWideChar(CP_ACP, 0, m_pchData,
		GetData()->nDataLength, NULL, NULL);
	BSTR bstr = ::SysAllocStringLen(NULL, nLen);
	if(bstr != NULL)
		MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength, bstr, nLen);
#endif
	return bstr;
}

inline BSTR CString::SetSysString(BSTR* pbstr) const
{
#if defined(_UNICODE) || defined(OLE2ANSI)
	::SysReAllocStringLen(pbstr, m_pchData, GetData()->nDataLength);
#else
	int nLen = MultiByteToWideChar(CP_ACP, 0, m_pchData,
		GetData()->nDataLength, NULL, NULL);
	if(::SysReAllocStringLen(pbstr, NULL, nLen))
		MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength, *pbstr, nLen);
#endif
	ATLASSERT(*pbstr != NULL);
	return *pbstr;
}
#endif //!_ATL_NO_COM

#endif //!_WTL_NO_CSTRING


/////////////////////////////////////////////////////////////////////////////
// CRecentDocumentList - MRU List Support

#ifndef _WTL_MRUEMPTY_TEXT
#define _WTL_MRUEMPTY_TEXT	_T("(empty)")
#endif

// forward declaration
inline bool AtlCompactPath(LPTSTR lpstrOut, LPCTSTR lpstrIn, int cchLen);

template <class T, int t_cchItemLen = MAX_PATH>
class CRecentDocumentListBase
{
public:
// Declarations
	struct _DocEntry
	{
		TCHAR szDocName[t_cchItemLen];
		bool operator==(const _DocEntry& de) const
		{ return (lstrcmpi(szDocName, de.szDocName) == 0); }
	};

	enum
	{
		m_nMaxEntries_Min = 2,
		m_nMaxEntries_Max = ID_FILE_MRU_LAST - ID_FILE_MRU_FIRST + 1,
		m_cchMaxItemLen_Min = 6,
		m_cchMaxItemLen_Max = t_cchItemLen
	};

// Data members
	CSimpleArray<_DocEntry> m_arrDocs;
	int m_nMaxEntries;	// default is 4
	HMENU m_hMenu;

	TCHAR m_szNoEntries[t_cchItemLen];

	int m_cchMaxItemLen;

// Constructor
	CRecentDocumentListBase() : m_hMenu(NULL), m_nMaxEntries(4), m_cchMaxItemLen(-1)
	{
		ATLASSERT(t_cchItemLen > m_cchMaxItemLen_Min);
	}

// Attributes
	HMENU GetMenuHandle() const
	{
		return m_hMenu;
	}
	void SetMenuHandle(HMENU hMenu)
	{
		ATLASSERT(hMenu == NULL || ::IsMenu(hMenu));
		m_hMenu = hMenu;
		if(m_hMenu == NULL || (::GetMenuString(m_hMenu, ID_FILE_MRU_FIRST, m_szNoEntries, t_cchItemLen, MF_BYCOMMAND) == 0))
			lstrcpy(m_szNoEntries, _WTL_MRUEMPTY_TEXT);
	}
	int GetMaxEntries() const
	{
		return m_nMaxEntries;
	}
	void SetMaxEntries(int nMaxEntries)
	{
		ATLASSERT(nMaxEntries > m_nMaxEntries_Min && nMaxEntries < m_nMaxEntries_Max);
		if(nMaxEntries < m_nMaxEntries_Min)
			nMaxEntries = m_nMaxEntries_Min;
		else if(nMaxEntries > m_nMaxEntries_Max)
			nMaxEntries = m_nMaxEntries_Max;
		m_nMaxEntries = nMaxEntries;
	}
	int GetMaxItemLength() const
	{
		return m_cchMaxItemLen;
	}
	void SetMaxItemLength(int cchMaxLen)
	{
		ATLASSERT((cchMaxLen >= m_cchMaxItemLen_Min && cchMaxLen <= m_cchMaxItemLen_Max) || cchMaxLen == -1);
		if(cchMaxLen != -1)
		{
			if(cchMaxLen < m_cchMaxItemLen_Min)
				cchMaxLen = m_cchMaxItemLen_Min;
			else if(cchMaxLen > m_cchMaxItemLen_Max)
				cchMaxLen = m_cchMaxItemLen_Max;
		}
		m_cchMaxItemLen = cchMaxLen;
		T* pT = static_cast<T*>(this);
		pT->UpdateMenu();
	}

// Operations
	BOOL AddToList(LPCTSTR lpstrDocName)
	{
		_DocEntry de;
		if(lstrcpy(de.szDocName, lpstrDocName) == NULL)
			return FALSE;

		for(int i = 0; i < m_arrDocs.GetSize(); i++)
		{
			if(lstrcmpi(m_arrDocs[i].szDocName, lpstrDocName) == 0)
			{
				m_arrDocs.RemoveAt(i);
				break;
			}
		}

		if(m_arrDocs.GetSize() == m_nMaxEntries)
			m_arrDocs.RemoveAt(0);

		BOOL bRet = m_arrDocs.Add(de);
		if(bRet)
		{
			T* pT = static_cast<T*>(this);
			bRet = pT->UpdateMenu();
		}
		return bRet;
	}
	BOOL GetFromList(int nItemID, LPTSTR lpstrDocName)
	{
		int nIndex = m_arrDocs.GetSize() - (nItemID - ID_FILE_MRU_FIRST) - 1;
		if(nIndex < 0 || nIndex >= m_arrDocs.GetSize())
			return FALSE;
		return (lstrcpy(lpstrDocName, m_arrDocs[nIndex].szDocName) != NULL);
	}
	BOOL RemoveFromList(int nItemID)
	{
		int nIndex = m_arrDocs.GetSize() - (nItemID - ID_FILE_MRU_FIRST) - 1;
		BOOL bRet = m_arrDocs.RemoveAt(nIndex);
		if(bRet)
		{
			T* pT = static_cast<T*>(this);
			bRet = pT->UpdateMenu();
		}
		return bRet;
	}
	BOOL MoveToTop(int nItemID)
	{
		int nIndex = m_arrDocs.GetSize() - 1 - (nItemID - ID_FILE_MRU_FIRST);
		if(nIndex < 0 || nIndex >= m_arrDocs.GetSize())
			return FALSE;
		_DocEntry de;
		de = m_arrDocs[nIndex];
		m_arrDocs.RemoveAt(nIndex);
		BOOL bRet = m_arrDocs.Add(de);
		if(bRet)
		{
			T* pT = static_cast<T*>(this);
			bRet = pT->UpdateMenu();
		}
		return bRet;
	}

	BOOL ReadFromRegistry(LPCTSTR lpstrRegKey)
	{
		CRegKey rkParent;
		CRegKey rk;

		LONG lRet = rkParent.Open(HKEY_CURRENT_USER, lpstrRegKey);
		if(lRet != ERROR_SUCCESS)
			return FALSE;
		lRet = rk.Open(rkParent, _T("Recent Document List"));
		if(lRet != ERROR_SUCCESS)
			return FALSE;

		DWORD dwRet;
#if (_ATL_VER >= 0x0700)
		lRet = rk.QueryDWORDValue(_T("DocumentCount"), dwRet);
#else
		lRet = rk.QueryValue(dwRet, _T("DocumentCount"));
#endif
		if(lRet != ERROR_SUCCESS)
			return FALSE;
		SetMaxEntries(dwRet);

		m_arrDocs.RemoveAll();

		TCHAR szRetString[t_cchItemLen];
		_DocEntry de;

		for(int nItem = m_nMaxEntries; nItem > 0; nItem--)
		{
			TCHAR szBuff[11];
			wsprintf(szBuff, _T("Document%i"), nItem);
#if (_ATL_VER >= 0x0700)
			ULONG ulCount = t_cchItemLen * sizeof(TCHAR);
			lRet = rk.QueryStringValue(szBuff, szRetString, &ulCount);
#else
			DWORD dwCount = t_cchItemLen * sizeof(TCHAR);
			lRet = rk.QueryValue(szRetString, szBuff, &dwCount);
#endif
			if(lRet == ERROR_SUCCESS && (lstrcpy(de.szDocName, szRetString) != NULL))
				m_arrDocs.Add(de);
		}

		rk.Close();
		rkParent.Close();

		T* pT = static_cast<T*>(this);
		return pT->UpdateMenu();
	}
	BOOL WriteToRegistry(LPCTSTR lpstrRegKey)
	{
		CRegKey rkParent;
		CRegKey rk;

		LONG lRet = rkParent.Create(HKEY_CURRENT_USER, lpstrRegKey);
		if(lRet != ERROR_SUCCESS)
			return FALSE;
		lRet = rk.Create(rkParent, _T("Recent Document List"));
		if(lRet != ERROR_SUCCESS)
			return FALSE;

#if (_ATL_VER >= 0x0700)
		lRet = rk.SetDWORDValue(_T("DocumentCount"), m_nMaxEntries);
#else
		lRet = rk.SetValue(m_nMaxEntries, _T("DocumentCount"));
#endif
		ATLASSERT(lRet == ERROR_SUCCESS);

		// set new values
		int nItem;
		for(nItem = m_arrDocs.GetSize(); nItem > 0; nItem--)
		{
			TCHAR szBuff[11];
			wsprintf(szBuff, _T("Document%i"), nItem);
			TCHAR szDocName[t_cchItemLen];
			GetFromList(ID_FILE_MRU_FIRST + nItem - 1, szDocName);
#if (_ATL_VER >= 0x0700)
			lRet = rk.SetStringValue(szBuff, szDocName);
#else
			lRet = rk.SetValue(szDocName, szBuff);
#endif
			ATLASSERT(lRet == ERROR_SUCCESS);
		}

		// delete unused keys
		for(nItem = m_arrDocs.GetSize() + 1; nItem < (ID_FILE_MRU_LAST - ID_FILE_MRU_FIRST + 1); nItem++)
		{
			TCHAR szBuff[11];
			wsprintf(szBuff, _T("Document%i"), nItem);
			rk.DeleteValue(szBuff);
		}

		rk.Close();
		rkParent.Close();

		return TRUE;
	}

// Implementation
	BOOL UpdateMenu()
	{
		if(m_hMenu == NULL)
			return FALSE;
		ATLASSERT(::IsMenu(m_hMenu));

		int nItems = ::GetMenuItemCount(m_hMenu);
		int nInsertPoint;
		for(nInsertPoint = 0; nInsertPoint < nItems; nInsertPoint++)
		{
			MENUITEMINFO mi;
			mi.cbSize = sizeof(MENUITEMINFO);
			mi.fMask = MIIM_ID;
			::GetMenuItemInfo(m_hMenu, nInsertPoint, TRUE, &mi);
			if (mi.wID == ID_FILE_MRU_FIRST)
				break;
		}
		ATLASSERT(nInsertPoint < nItems && "You need a menu item with an ID = ID_FILE_MRU_FIRST");

		int nItem;
		for(nItem = ID_FILE_MRU_FIRST; nItem < ID_FILE_MRU_FIRST + m_nMaxEntries; nItem++)
		{
			// keep the first one as an insertion point
			if (nItem != ID_FILE_MRU_FIRST)
				::DeleteMenu(m_hMenu, nItem, MF_BYCOMMAND);
		}

		TCHAR szItemText[t_cchItemLen + 6];		// add space for &, 2 digits, and a space
		int nSize = m_arrDocs.GetSize();
		nItem = 0;
		if(nSize > 0)
		{
			for(nItem = 0; nItem < nSize; nItem++)
			{
				if(m_cchMaxItemLen == -1)
				{
					wsprintf(szItemText, _T("&%i %s"), nItem + 1, m_arrDocs[nSize - 1 - nItem].szDocName);
				}
				else
				{
					TCHAR szBuff[t_cchItemLen];
					T* pT = static_cast<T*>(this);
					pT;	// avoid level 4 warning
					bool bRet = pT->CompactDocumentName(szBuff, m_arrDocs[nSize - 1 - nItem].szDocName, m_cchMaxItemLen);
					bRet;	// avoid level 4 warning
					ATLASSERT(bRet);
					wsprintf(szItemText, _T("&%i %s"), nItem + 1, szBuff);
				}
				::InsertMenu(m_hMenu, nInsertPoint + nItem, MF_BYPOSITION | MF_STRING, ID_FILE_MRU_FIRST + nItem, szItemText);
			}
		}
		else	// empty
		{
			::InsertMenu(m_hMenu, nInsertPoint, MF_BYPOSITION | MF_STRING, ID_FILE_MRU_FIRST, m_szNoEntries);
			::EnableMenuItem(m_hMenu, ID_FILE_MRU_FIRST, MF_GRAYED);
			nItem++;
		}
		::DeleteMenu(m_hMenu, nInsertPoint + nItem, MF_BYPOSITION);

		return TRUE;
	}

// Overrideable
	// override to provide a different method of compacting document names
	static bool CompactDocumentName(LPTSTR lpstrOut, LPCTSTR lpstrIn, int cchLen)
	{
		return AtlCompactPath(lpstrOut, lpstrIn, cchLen);
	}
};

class CRecentDocumentList : public CRecentDocumentListBase<CRecentDocumentList>
{
public:
// nothing here
};


/////////////////////////////////////////////////////////////////////////////
// CFindFile - file search helper class

class CFindFile
{
public:
// Data members
	WIN32_FIND_DATA m_fd;
	TCHAR m_lpszRoot[MAX_PATH];
	TCHAR m_chDirSeparator;
	HANDLE m_hFind;
	BOOL m_bFound;

// Constructor/destructor
	CFindFile() : m_hFind(NULL), m_chDirSeparator('\\'), m_bFound(FALSE)
	{ }

	~CFindFile()
	{
		Close();
	}

// Attributes
	ULONGLONG GetFileSize() const
	{
		ATLASSERT(m_hFind != NULL);

		ULARGE_INTEGER nFileSize;

		if(m_bFound)
		{
			nFileSize.LowPart = m_fd.nFileSizeLow;
			nFileSize.HighPart = m_fd.nFileSizeHigh;
		}
		else
		{
			nFileSize.QuadPart = 0;
		}

		return nFileSize.QuadPart;
	}
	BOOL GetFileName(LPTSTR lpstrFileName, int cchLength) const
	{
		ATLASSERT(m_hFind != NULL);
		if(lstrlen(m_fd.cFileName) >= cchLength)
			return FALSE;
		return (m_bFound && (lstrcpy(lpstrFileName, m_fd.cFileName) != NULL));
	}
	BOOL GetFilePath(LPTSTR lpstrFilePath, int cchLength) const
	{
		ATLASSERT(m_hFind != NULL);

		int nLen = lstrlen(m_lpszRoot);
		ATLASSERT(nLen > 0);
		if(nLen == 0)
			return FALSE;
		bool bAddSep = (m_lpszRoot[nLen - 1] != '\\' && m_lpszRoot[nLen - 1] != '/');

		if((lstrlen(m_lpszRoot) + (bAddSep ?  1 : 0)) >= cchLength)
			return FALSE;

		BOOL bRet = (lstrcpy(lpstrFilePath, m_lpszRoot) != NULL);
		if(bRet)
		{
			TCHAR szSeparator[2] = { m_chDirSeparator, 0 };
			bRet = (lstrcat(lpstrFilePath, szSeparator) != NULL);
			if(bRet)
				bRet = (lstrcat(lpstrFilePath, m_fd.cFileName) != NULL);
		}
		return bRet;
	}
	BOOL GetFileTitle(LPTSTR lpstrFileTitle, int cchLength) const
	{
		ATLASSERT(m_hFind != NULL);

		TCHAR szBuff[MAX_PATH];
		if(!GetFileName(szBuff, MAX_PATH))
			return FALSE;
		TCHAR szNameBuff[_MAX_FNAME];
		_tsplitpath(szBuff, NULL, NULL, szNameBuff, NULL);
		if(lstrlen(szNameBuff) >= cchLength)
			return FALSE;
		return (lstrcpy(lpstrFileTitle, szNameBuff) != NULL);
	}
	BOOL GetFileURL(LPTSTR lpstrFileURL, int cchLength) const
	{
		ATLASSERT(m_hFind != NULL);

		TCHAR szBuff[MAX_PATH];
		if(!GetFilePath(szBuff, MAX_PATH))
			return FALSE;
		LPCTSTR lpstrFileURLPrefix = _T("file://");
		if(lstrlen(szBuff) + lstrlen(lpstrFileURLPrefix) >= cchLength)
			return FALSE;
		if(lstrcpy(lpstrFileURL, lpstrFileURLPrefix) == NULL)
			return FALSE;
		return (lstrcat(lpstrFileURL, szBuff) != NULL);
	}
	BOOL GetRoot(LPTSTR lpstrRoot, int cchLength) const
	{
		ATLASSERT(m_hFind != NULL);
		if(lstrlen(m_lpszRoot) >= cchLength)
			return FALSE;
		return (lstrcpy(lpstrRoot, m_lpszRoot) != NULL);
	}
#if defined(_WTL_USE_CSTRING) || defined(__ATLSTR_H__)
	CString GetFileName() const
	{
		ATLASSERT(m_hFind != NULL);

		CString ret;

		if(m_bFound)
			ret = m_fd.cFileName;
		return ret;
	}
	CString GetFilePath() const
	{
		ATLASSERT(m_hFind != NULL);

		CString strResult = m_lpszRoot;
		if(strResult[strResult.GetLength() - 1] != '\\' &&
			strResult[strResult.GetLength() - 1] != '/')
			strResult += m_chDirSeparator;
		strResult += GetFileName();
		return strResult;
	}
	CString GetFileTitle() const
	{
		ATLASSERT(m_hFind != NULL);

		CString strFullName = GetFileName();
		CString strResult;

		_tsplitpath(strFullName, NULL, NULL, strResult.GetBuffer(MAX_PATH), NULL);
		strResult.ReleaseBuffer();
		return strResult;
	}
	CString GetFileURL() const
	{
		ATLASSERT(m_hFind != NULL);

		CString strResult("file://");
		strResult += GetFilePath();
		return strResult;
	}
	CString GetRoot() const
	{
		ATLASSERT(m_hFind != NULL);

		CString str = m_lpszRoot;
		return str;
	}
#endif //defined(_WTL_USE_CSTRING) || defined(__ATLSTR_H__)
	BOOL GetLastWriteTime(FILETIME* pTimeStamp) const
	{
		ATLASSERT(m_hFind != NULL);
		ATLASSERT(pTimeStamp != NULL);

		if(m_bFound && pTimeStamp != NULL)
		{
			*pTimeStamp = m_fd.ftLastWriteTime;
			return TRUE;
		}

		return FALSE;
	}
	BOOL GetLastAccessTime(FILETIME* pTimeStamp) const
	{
		ATLASSERT(m_hFind != NULL);
		ATLASSERT(pTimeStamp != NULL);

		if(m_bFound && pTimeStamp != NULL)
		{
			*pTimeStamp = m_fd.ftLastAccessTime;
			return TRUE;
		}

		return FALSE;
	}
	BOOL GetCreationTime(FILETIME* pTimeStamp) const
	{
		ATLASSERT(m_hFind != NULL);

		if(m_bFound && pTimeStamp != NULL)
		{
			*pTimeStamp = m_fd.ftCreationTime;
			return TRUE;
		}

		return FALSE;
	}
	BOOL MatchesMask(DWORD dwMask) const
	{
		ATLASSERT(m_hFind != NULL);

		if(m_bFound)
			return ((m_fd.dwFileAttributes & dwMask) != 0);

		return FALSE;
	}
	BOOL IsDots() const
	{
		ATLASSERT(m_hFind != NULL);

		// return TRUE if the file name is "." or ".." and
		// the file is a directory

		BOOL bResult = FALSE;
		if(m_bFound && IsDirectory())
		{
			if(m_fd.cFileName[0] == '.' && (m_fd.cFileName[1] == '\0' || (m_fd.cFileName[1] == '.' && m_fd.cFileName[2] == '\0')))
				bResult = TRUE;
		}

		return bResult;
	}

	BOOL IsReadOnly() const
	{
		return MatchesMask(FILE_ATTRIBUTE_READONLY);
	}
	BOOL IsDirectory() const
	{
		return MatchesMask(FILE_ATTRIBUTE_DIRECTORY);
	}
	BOOL IsCompressed() const
	{
		return MatchesMask(FILE_ATTRIBUTE_COMPRESSED);
	}
	BOOL IsSystem() const
	{
		return MatchesMask(FILE_ATTRIBUTE_SYSTEM);
	}
	BOOL IsHidden() const
	{
		return MatchesMask(FILE_ATTRIBUTE_HIDDEN);
	}
	BOOL IsTemporary() const
	{
		return MatchesMask(FILE_ATTRIBUTE_TEMPORARY);
	}
	BOOL IsNormal() const
	{
		return MatchesMask(FILE_ATTRIBUTE_NORMAL);
	}
	BOOL IsArchived() const
	{
		return MatchesMask(FILE_ATTRIBUTE_ARCHIVE);
	}

// Operations
	BOOL FindFile(LPCTSTR pstrName = NULL)
	{
		Close();

		if(pstrName == NULL)
			pstrName = _T("*.*");
		lstrcpy(m_fd.cFileName, pstrName);

		m_hFind = ::FindFirstFile(pstrName, &m_fd);

		if(m_hFind == INVALID_HANDLE_VALUE)
			return FALSE;

		LPCTSTR pstr = _tfullpath(m_lpszRoot, pstrName, MAX_PATH);

		// passed name isn't a valid path but was found by the API
		ATLASSERT(pstr != NULL);
		if(pstr == NULL)
		{
			Close();
			::SetLastError(ERROR_INVALID_NAME);
			return FALSE;
		}
		else
		{
			// find the last forward or backward whack
			LPTSTR pstrBack  = _tcsrchr(m_lpszRoot, '\\');
			LPTSTR pstrFront = _tcsrchr(m_lpszRoot, '/');

			if(pstrFront != NULL || pstrBack != NULL)
			{
				if(pstrFront == NULL)
					pstrFront = m_lpszRoot;
				if(pstrBack == NULL)
					pstrBack = m_lpszRoot;

				// from the start to the last whack is the root

				if(pstrFront >= pstrBack)
					*pstrFront = '\0';
				else
					*pstrBack = '\0';
			}
		}

		m_bFound = TRUE;

		return TRUE;
	}

	BOOL FindNextFile()
	{
		ATLASSERT(m_hFind != NULL);

		if(m_hFind == NULL)
			return FALSE;

		if(!m_bFound)
			return FALSE;

		m_bFound = ::FindNextFile(m_hFind, &m_fd);

		return m_bFound;
	}
	void Close()
	{
		m_bFound = FALSE;

		if(m_hFind != NULL && m_hFind != INVALID_HANDLE_VALUE)
		{
			::FindClose(m_hFind);
			m_hFind = NULL;
		}
	}
};


/////////////////////////////////////////////////////////////////////////////
// Global functions for loading resources

inline HACCEL AtlLoadAccelerators(_U_STRINGorID table)
{
	return ::LoadAccelerators(_Module.GetResourceInstance(), table.m_lpstr);
}

inline HMENU AtlLoadMenu(_U_STRINGorID menu)
{
	return ::LoadMenu(_Module.GetResourceInstance(), menu.m_lpstr);
}

inline HBITMAP AtlLoadBitmap(_U_STRINGorID bitmap)
{
	return ::LoadBitmap(_Module.GetResourceInstance(), bitmap.m_lpstr);
}

#ifdef OEMRESOURCE
inline HBITMAP AtlLoadSysBitmap(LPCTSTR lpBitmapName)
{
#ifdef _DEBUG
	WORD wID = (WORD)lpBitmapName;
	ATLASSERT(wID >= 32734 && wID <= 32767);
#endif //_DEBUG
	return ::LoadBitmap(NULL, lpBitmapName);
}
#endif //OEMRESOURCE

inline HCURSOR AtlLoadCursor(_U_STRINGorID cursor)
{
	return ::LoadCursor(_Module.GetResourceInstance(), cursor.m_lpstr);
}

inline HCURSOR AtlLoadSysCursor(LPCTSTR lpCursorName)
{
	ATLASSERT(lpCursorName == IDC_ARROW || lpCursorName == IDC_IBEAM || lpCursorName == IDC_WAIT ||
		lpCursorName == IDC_CROSS || lpCursorName == IDC_UPARROW || lpCursorName == IDC_SIZE ||
		lpCursorName == IDC_ICON || lpCursorName == IDC_SIZENWSE || lpCursorName == IDC_SIZENESW ||
		lpCursorName == IDC_SIZEWE || lpCursorName == IDC_SIZENS || lpCursorName == IDC_SIZEALL ||
		lpCursorName == IDC_NO || lpCursorName == IDC_APPSTARTING || lpCursorName == IDC_HELP);
	return ::LoadCursor(NULL, lpCursorName);
}

inline HICON AtlLoadIcon(_U_STRINGorID icon)
{
	return ::LoadIcon(_Module.GetResourceInstance(), icon.m_lpstr);
}

inline HICON AtlLoadSysIcon(LPCTSTR lpIconName)
{
	ATLASSERT(lpIconName == IDI_APPLICATION ||
		lpIconName == IDI_ASTERISK ||
		lpIconName == IDI_EXCLAMATION ||
		lpIconName == IDI_HAND ||
		lpIconName == IDI_QUESTION ||
		lpIconName == IDI_WINLOGO);
	return ::LoadIcon(NULL, lpIconName);
}

inline HBITMAP AtlLoadBitmapImage(_U_STRINGorID bitmap, UINT fuLoad = LR_DEFAULTCOLOR)
{
	return (HBITMAP)::LoadImage(_Module.GetResourceInstance(), bitmap.m_lpstr, IMAGE_BITMAP, 0, 0, fuLoad);
}

inline HCURSOR AtlLoadCursorImage(_U_STRINGorID cursor, UINT fuLoad = LR_DEFAULTCOLOR | LR_DEFAULTSIZE, int cxDesired = 0, int cyDesired = 0)
{
	return (HCURSOR)::LoadImage(_Module.GetResourceInstance(), cursor.m_lpstr, IMAGE_CURSOR, cxDesired, cyDesired, fuLoad);
}

inline HICON AtlLoadIconImage(_U_STRINGorID icon, UINT fuLoad = LR_DEFAULTCOLOR | LR_DEFAULTSIZE, int cxDesired = 0, int cyDesired = 0)
{
	return (HICON)::LoadImage(_Module.GetResourceInstance(), icon.m_lpstr, IMAGE_ICON, cxDesired, cyDesired, fuLoad);
}

#ifdef OEMRESOURCE
inline HBITMAP AtlLoadSysBitmapImage(WORD wBitmapID, UINT fuLoad = LR_DEFAULTCOLOR)
{
	ATLASSERT(wBitmapID >= 32734 && wBitmapID <= 32767);
	ATLASSERT((fuLoad & LR_LOADFROMFILE) == 0);	// this one doesn't load from a file
	return (HBITMAP)::LoadImage(NULL, MAKEINTRESOURCE(wBitmapID), IMAGE_BITMAP, 0, 0, fuLoad);
}
#endif //OEMRESOURCE

inline HCURSOR AtlLoadSysCursorImage(_U_STRINGorID cursor, UINT fuLoad = LR_DEFAULTCOLOR | LR_DEFAULTSIZE, int cxDesired = 0, int cyDesired = 0)
{
#ifdef _DEBUG
	WORD wID = (WORD)cursor.m_lpstr;
	ATLASSERT((wID >= 32512 && wID <= 32516) || (wID >= 32640 && wID <= 32648) || (wID == 32650) || (wID == 32651));
	ATLASSERT((fuLoad & LR_LOADFROMFILE) == 0);	// this one doesn't load from a file
#endif //_DEBUG
	return (HCURSOR)::LoadImage(NULL, cursor.m_lpstr, IMAGE_CURSOR, cxDesired, cyDesired, fuLoad);
}

inline HICON AtlLoadSysIconImage(_U_STRINGorID icon, UINT fuLoad = LR_DEFAULTCOLOR | LR_DEFAULTSIZE, int cxDesired = 0, int cyDesired = 0)
{
#ifdef _DEBUG
	WORD wID = (WORD)icon.m_lpstr;
	ATLASSERT(wID >= 32512 && wID <= 32517);
	ATLASSERT((fuLoad & LR_LOADFROMFILE) == 0);	// this one doesn't load from a file
#endif //_DEBUG
	return (HICON)::LoadImage(NULL, icon.m_lpstr, IMAGE_ICON, cxDesired, cyDesired, fuLoad);
}

#if (_ATL_VER < 0x0700)
inline int AtlLoadString(UINT uID, LPTSTR lpBuffer, int nBufferMax)
{
	return ::LoadString(_Module.GetResourceInstance(), uID, lpBuffer, nBufferMax);
}
#endif //(_ATL_VER < 0x0700)

inline bool AtlLoadString(UINT uID, BSTR& bstrText)
{
	USES_CONVERSION;
	ATLASSERT(bstrText == NULL);

	LPTSTR lpstrText = NULL;
	int nRes = 0;
	for(int nLen = 256; ; nLen *= 2)
	{
		ATLTRY(lpstrText = new TCHAR[nLen]);
		if(lpstrText == NULL)
			break;
		nRes = ::LoadString(_Module.GetResourceInstance(), uID, lpstrText, nLen);
		if(nRes < nLen - 1)
			break;
		delete [] lpstrText;
		lpstrText = NULL;
	}

	if(lpstrText != NULL)
	{
		if(nRes != 0)
			bstrText = ::SysAllocString(T2OLE(lpstrText));
		delete [] lpstrText;
	}

	return (bstrText != NULL) ? true : false;
}


/////////////////////////////////////////////////////////////////////////////
// Global functions for stock GDI objects

inline HPEN AtlGetStockPen(int nPen)
{
#if (_WIN32_WINNT >= 0x0500)
	ATLASSERT(nPen == WHITE_PEN || nPen == BLACK_PEN || nPen == NULL_PEN || nPen == DC_PEN);
#else
	ATLASSERT(nPen == WHITE_PEN || nPen == BLACK_PEN || nPen == NULL_PEN);
#endif //!(_WIN32_WINNT >= 0x0500)
	return (HPEN)::GetStockObject(nPen);
}

inline HBRUSH AtlGetStockBrush(int nBrush)
{
#if (_WIN32_WINNT >= 0x0500)
	ATLASSERT((nBrush >= WHITE_BRUSH && nBrush <= HOLLOW_BRUSH) || nBrush == DC_BRUSH);
#else
	ATLASSERT(nBrush >= WHITE_BRUSH && nBrush <= HOLLOW_BRUSH);
#endif //!(_WIN32_WINNT >= 0x0500)
	return (HBRUSH)::GetStockObject(nBrush);
}

inline HFONT AtlGetStockFont(int nFont)
{
	ATLASSERT((nFont >= OEM_FIXED_FONT && nFont <= SYSTEM_FIXED_FONT) || nFont == DEFAULT_GUI_FONT);
	return (HFONT)::GetStockObject(nFont);
}

inline HPALETTE AtlGetStockPalette(int nPalette)
{
	ATLASSERT(nPalette == DEFAULT_PALETTE); // the only one supported
	return (HPALETTE)::GetStockObject(nPalette);
}


/////////////////////////////////////////////////////////////////////////////
// Global function for compacting a path by replacing parts with ellipsis

// helper for multi-byte character sets
inline bool _IsDBCSTrailByte(LPCTSTR lpstr, int nChar)
{
#ifndef _UNICODE
	for(int i = nChar; i > 0; i--)
	{
		if(!::IsDBCSLeadByte(lpstr[i - 1]))
			break;
	}
	return ((nChar > 0) && (((nChar - i) & 1) != 0));
#else //_UNICODE
	lpstr;	nChar;
	return false;
#endif //_UNICODE
}

inline bool AtlCompactPath(LPTSTR lpstrOut, LPCTSTR lpstrIn, int cchLen)
{
	ATLASSERT(lpstrOut != NULL);
	ATLASSERT(lpstrIn != NULL);
	ATLASSERT(cchLen > 0);

	LPCTSTR szEllipsis = _T("...");
	const int cchEndEllipsis = 3;
	const int cchMidEllipsis = 4;

	if(lstrlen(lpstrIn) + 1 < cchLen)
		return (lstrcpy(lpstrOut, lpstrIn) != NULL);

	lpstrOut[0] = 0;

	// check if the separator is a slash or a backslash
	TCHAR chSlash = _T('\\');
	for(LPTSTR lpstr = (LPTSTR)lpstrIn; *lpstr != 0; lpstr = ::CharNext(lpstr))
	{
		if((*lpstr == _T('/')) || (*lpstr == _T('\\')))
			chSlash = *lpstr;
	}

	// find the filename portion of the path
	LPCTSTR lpstrFileName = lpstrIn;
	for(LPCTSTR pPath = lpstrIn; *pPath; pPath = ::CharNext(pPath))
	{
		if((pPath[0] == _T('\\') || pPath[0] == _T(':') || pPath[0] == _T('/'))
				&& pPath[1] && pPath[1] != _T('\\') && pPath[1] != _T('/'))
			lpstrFileName = pPath + 1;
	}
	int cchFileName = lstrlen(lpstrFileName);

	// handle just the filename without a path
	if(lpstrFileName == lpstrIn && cchLen > cchEndEllipsis)
	{
		bool bRet = (lstrcpyn(lpstrOut, lpstrIn, cchLen - cchEndEllipsis) != NULL);
		if(bRet)
		{
#ifndef _UNICODE
			if(_IsDBCSTrailByte(lpstrIn, cchLen - cchEndEllipsis))
				lpstrOut[cchLen - cchEndEllipsis - 1] = 0;
#endif //_UNICODE
			bRet = (lstrcat(lpstrOut, szEllipsis) != NULL);
		}
		return bRet;
	}

	// handle just ellipsis
	if((cchLen < (cchMidEllipsis + cchEndEllipsis)))
	{
		for(int i = 0; i < cchLen - 1; i++)
			lpstrOut[i] = ((i + 1) == cchMidEllipsis) ? chSlash : _T('.');
		lpstrOut[i] = 0;
		return true;
	}

	// calc how much we have to copy
	int cchToCopy = cchLen - (cchMidEllipsis + cchFileName);

	if(cchToCopy < 0)
		cchToCopy = 0;

#ifndef _UNICODE
	if(cchToCopy > 0 && _IsDBCSTrailByte(lpstrIn, cchToCopy))
		cchToCopy--;
#endif //_UNICODE

	bool bRet = (lstrcpyn(lpstrOut, lpstrIn, cchToCopy) != NULL);
	if(!bRet)
		return false;

	// add ellipsis
	bRet = (lstrcat(lpstrOut, szEllipsis) != NULL);
	if(!bRet)
		return false;
	TCHAR szSlash[2] = { chSlash, 0 };
	bRet = (lstrcat(lpstrOut, szSlash) != NULL);
	if(!bRet)
		return false;

	// add filename (and ellipsis, if needed)
	if(cchLen > (cchMidEllipsis + cchFileName))
	{
		bRet = (lstrcat(lpstrOut, lpstrFileName) != NULL);
	}
	else
	{
		cchToCopy = cchLen - cchMidEllipsis - cchEndEllipsis;
#ifndef _UNICODE
		if(cchToCopy > 0 && _IsDBCSTrailByte(lpstrFileName, cchToCopy))
			cchToCopy--;
#endif //_UNICODE
		bRet = (lstrcpyn(&lpstrOut[cchMidEllipsis], lpstrFileName, cchToCopy) != NULL);
		if(bRet)
			bRet = (lstrcat(lpstrOut, szEllipsis) != NULL);
	}

	return bRet;
}

}; //namespace WTL

#endif // __ATLMISC_H__
