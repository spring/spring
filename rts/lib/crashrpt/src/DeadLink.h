///////////////////////////////////////////////////////////////////////////////
//
//  Module: DeadLink.h
//
//    Desc: CDeadLink gives the appearance of a hyperlink control, but when
//          clicked it send a WM_DEADLINKCLICK message instead of opening
//          a URL.  Basically, it's just a button that looks like a hyperlink.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DEADLINK_H_
#define _DEADLINK_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <atlctrls.h>   // Common controls
#include <atlctrlx.h>   // CHyperlink

// Custom click message
#define DL_CLICKED 0


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CDeadLink
// 
// See the module comment at top of file.
//
class CDeadLink : public CHyperLinkImpl<CDeadLink>
{
public:
   BEGIN_MSG_MAP(CDeadLink)
      MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
      CHAIN_MSG_MAP(CHyperLinkImpl<CDeadLink>)
   END_MSG_MAP()

   
   //-----------------------------------------------------------------------------
   // OnLButtonDown
   //
   // Send message to parent control instead of trying to open URL.
   //
   LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      ::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), DL_CLICKED), (LPARAM)m_hWnd);
      return 0;
   }
};

#endif	// #ifndef _DEADLINK_H_
