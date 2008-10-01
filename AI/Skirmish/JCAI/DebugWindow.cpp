//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
//
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------

#include "DebugWindow.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h> 

void DbgWndPrintf (DbgWindow* wnd, int x,int y, const char *fmt, ...)
{
	static char buf[256];
	va_list vl;
	va_start (vl, fmt);
#ifdef _MSC_VER
	_vsnprintf (buf, sizeof(buf), fmt, vl);
#else
	vsnprintf (buf, sizeof(buf), fmt, vl);
#endif
	DbgWindowTextout (wnd, x,y,buf);
	va_end (vl);
}


#ifdef WIN32
#include <windows.h>

typedef unsigned char uchar;

class DbgWindow
{
public:
	DbgWindow () {bClosed=false;buffer=0; hWnd=0;dc=0;}
	~DbgWindow ();

	bool Init (const char *name, int w,int h);
	void Update(HDC dc=0);

	static void RegClass ();
	static void UnregClass ();
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	int width, height;
	HWND hWnd;
	bool bClosed;
	HBITMAP buffer;
	HDC dc,hdcMem;
	uchar *img;
	int imgW, imgH;

	static int clsRegs;
	static const char *WndClassName;
};

int DbgWindow::clsRegs = 0;
const char* DbgWindow::WndClassName = "DebugWindow";

LRESULT CALLBACK DbgWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_CLOSE:{
		DestroyWindow(hWnd);
		DbgWindow *wnd = (DbgWindow *)GetWindowLong (hWnd, GWL_USERDATA);
		wnd->bClosed = true;
		wnd->hWnd = 0;
		UnregClass ();
		break;}
	case WM_ACTIVATE:
		ShowCursor(TRUE);
		break;
	case WM_PAINT:{
		DbgWindow *wnd = (DbgWindow *)GetWindowLong (hWnd, GWL_USERDATA);
		if (wnd){
			PAINTSTRUCT ps;
			wnd->Update (BeginPaint (hWnd,&ps));
			EndPaint (hWnd, &ps);
		}
		break;}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool DbgWindow::Init(const char *name, int w,int h)
{
	width = w;
	height = h;

	RegClass ();

	int scrw = GetSystemMetrics(SM_CXSCREEN);

	hWnd = CreateWindowEx(WS_EX_TOPMOST, WndClassName, name, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
		scrw-w, 0, w,h, NULL, NULL, GetModuleHandle(0), NULL);

	if(!hWnd)
		return false;

	SetWindowLong (hWnd, GWL_USERDATA,(LONG)this);

	ShowWindow(hWnd,SW_SHOW);
	UpdateWindow(hWnd);
	
	RECT rc;
	GetClientRect (hWnd, &rc);

	imgW = rc.right;
	imgH = rc.bottom;

	dc = GetDC (hWnd);

	void *bmpData = 0;
	buffer = CreateCompatibleBitmap (dc, imgW, imgH);
	if (!buffer)
	{
		MessageBox (hWnd, "Failed to create DIB section for drawing.", "Error:",MB_OK|MB_ICONERROR);
		return false;
	}

	img = (uchar *)bmpData;

	hdcMem = CreateCompatibleDC( NULL );
	SelectObject( hdcMem, buffer );

	return true;
}

DbgWindow::~DbgWindow()
{
	if (hdcMem)
	{
		DeleteDC(hdcMem);
		hdcMem=0;
	}

	if (buffer)
	{
		DeleteObject (buffer);
		buffer = 0;
	}

	if (dc) 
	{
		ReleaseDC(hWnd, dc);
		dc=0;
	}

	if (hWnd) {
		DestroyWindow (hWnd);
		hWnd = 0;
		UnregClass ();
	}
}

void DbgWindow::RegClass()
{
	if (!clsRegs)
	{
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX); 
		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= WndProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= (HINSTANCE)GetModuleHandle(0);
		wcex.hIcon			= 0;
		wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground	= 0;
		wcex.lpszMenuName	= 0;
		wcex.lpszClassName	= WndClassName;
		wcex.hIconSm		= 0;

		RegisterClassEx(&wcex);
	}
	clsRegs++;
}

void DbgWindow::UnregClass ()
{
	clsRegs --;
	if (!clsRegs) {
		UnregisterClass (WndClassName, (HINSTANCE)GetModuleHandle(0));
	}
}

void DbgWindow::Update(HDC pdc)
{
	BitBlt( pdc ? pdc : dc, 0, 0, imgW, imgH, hdcMem, 0, 0, SRCCOPY );
}


DbgWindow* DbgCreateWindow (const char *name, int w,int h)
{
	DbgWindow *wnd = new DbgWindow;

	if (!wnd->Init (name,w,h))
	{
		delete wnd;
		return 0;
	}

	return wnd;
}

void DbgDestroyWindow(DbgWindow *wnd)
{
	delete wnd;
}

void DbgClearWindow(DbgWindow *wnd)
{
	BitBlt(wnd->hdcMem,0,0,wnd->imgW,wnd->imgH,0,0,0,WHITENESS);
}

void DbgWindowTextout (DbgWindow *wnd, int x,int y, const char *text)
{
	if (wnd->bClosed)
		return;

	TextOut (wnd->hdcMem, x,y,text,strlen(text));
}

void DbgWindowPutpixel (DbgWindow *wnd, int x,int y, int col)
{
	if (wnd->bClosed)
		return;

/*		p[0] = col & 0xff;
		p[1] = (col & 0xff00) >> 8;
		p[2] = (col & 0xff0000) >> 16;*/
	SetPixel (wnd->hdcMem, x,y, col);
}

void DbgWindowRect (DbgWindow *wnd, int x1,int y1, int x2,int y2,int col)
{
	static HBRUSH br = 0;
	static int cur;

	if (!br || col != cur)
	{
		if (br) DeleteObject (br);
		br = CreateSolidBrush (col);
		cur = col;
	}

	HGDIOBJ prev = (HBRUSH)SelectObject (wnd->hdcMem, br);
	Rectangle (wnd->hdcMem, x1,y1,x2,y2);
	SelectObject (wnd->hdcMem, prev);
}

// Copy the buffer to 
void DbgWindowUpdate (DbgWindow *wnd)
{
	wnd->Update ();
}


// 24 bit data
void DbgWindowDrawBitmap (DbgWindow *wnd, int x,int y,int w,int h, char *data)
{
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(bmi);
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = -h;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biSizeImage = w * h * 3;

	SetDIBitsToDevice (wnd->hdcMem,x,y,w,h,0,0,0,h,(void*)data,(BITMAPINFO*)&bmi,DIB_RGB_COLORS);
}

/* BROKEN */
void DbgWindowStretchBitmap (DbgWindow *wnd, int dstx,int dsty,int dstw,
							 int dsth, int srcw,int srch,char *data)
{
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(bmi);
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biWidth = srcw;
	bmi.bmiHeader.biHeight = -srch;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biSizeImage = srcw * srch * 3;

	StretchDIBits (wnd->hdcMem,dstx,dsty,dstw,dsth,0,0,srcw,srch,data,(BITMAPINFO*)&bmi, DIB_RGB_COLORS,SRCCOPY);
}

bool DbgWindowIsClosed (DbgWindow *wnd) {
	return wnd->bClosed;
}

#else

// empty version for anything not windows

class DbgWindow
{};

DbgWindow* DbgCreateWindow (const char *name, int w,int h)
{
	static DbgWindow wnd;
	return &wnd;
}

void DbgDestroyWindow (DbgWindow *wnd) {}
void DbgClearWindow (DbgWindow *wnd) {}
void DbgWindowTextout (DbgWindow *wnd, int x,int y,const char *text) {}
void DbgWindowPutpixel (DbgWindow *wnd, int x,int y, int col) {}
void DbgWindowRect (DbgWindow *wnd, int x1,int y1,int x2,int y2,int col) {}
void DbgWindowUpdate(DbgWindow *wnd) {}
void DbgWindowDrawBitmap (DbgWindow *wnd, int x,int y,int w,int h, char *data) {}
void DbgWindowStretchBitmap (DbgWindow *wnd, int dstx,int dsty,int dstw,int dsth, int srcw,int srch,char *data) {}
bool DbgWindowIsClosed(DbgWindow *wnd) { return true; }

#endif

