//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
/*
DebugWindow.cpp

Debug functions to open a drawing window next to the application window
very useful for showing debug output

Multiple windows can be opened at a time
*/
class DbgWindow;

DbgWindow* DbgCreateWindow (const char *name, int w,int h);
void DbgDestroyWindow (DbgWindow *wnd);
void DbgClearWindow (DbgWindow *wnd);
void DbgWindowTextout (DbgWindow *wnd, int x,int y,const char *text);
void DbgWndPrintf (DbgWindow *wnd, int x,int y,const char *fmt,...);
void DbgWindowPutpixel (DbgWindow *wnd, int x,int y, int col);
void DbgWindowRect (DbgWindow *wnd, int x1,int y1,int x2,int y2,int col);
void DbgWindowUpdate(DbgWindow *wnd);
void DbgWindowDrawBitmap (DbgWindow *wnd, int x,int y,int w,int h, char *data);
void DbgWindowStretchBitmap (DbgWindow *wnd, int dstx,int dsty,int dstw,int dsth, int srcw,int srch,char *data);
bool DbgWindowIsClosed (DbgWindow *wnd);
