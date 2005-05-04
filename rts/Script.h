// Script.h: interface for the CScript class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCRIPT_H__101D5001_6D52_11D5_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_SCRIPT_H__101D5001_6D52_11D5_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Object.h"
#include <string>

class CScript : public CObject  
{
public:
	virtual void SetCamera();
	virtual void Update();
	CScript(const std::string& name);
	virtual ~CScript();

	virtual std::string GetMapName(void);

	bool wantCameraControl;
	bool onlySinglePlayer;
	bool loadGame;
};

#endif // !defined(AFX_SCRIPT_H__101D5001_6D52_11D5_AD55_0080ADA84DE3__INCLUDED_)

