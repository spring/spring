#ifndef TESTSCRIPT_H
#define TESTSCRIPT_H
// TestScript.h: interface for the CTestScript class.
//
//////////////////////////////////////////////////////////////////////

#include "Script.h"

class CTestScript : public CScript  
{
public:
	virtual void Update();
	CTestScript();
	virtual ~CTestScript();

};

#endif /* TESTSCRIPT_H */
