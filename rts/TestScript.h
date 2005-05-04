// TestScript.h: interface for the CTestScript class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TESTSCRIPT_H__23859659_2F12_4B30_BD00_21288D5009BE__INCLUDED_)
#define AFX_TESTSCRIPT_H__23859659_2F12_4B30_BD00_21288D5009BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Script.h"

class CTestScript : public CScript  
{
public:
	virtual void Update();
	CTestScript();
	virtual ~CTestScript();

};

#endif // !defined(AFX_TESTSCRIPT_H__23859659_2F12_4B30_BD00_21288D5009BE__INCLUDED_)
