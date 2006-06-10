#ifndef SCRIPT_H
#define SCRIPT_H
// Script.h: interface for the CScript class.
//
//////////////////////////////////////////////////////////////////////

#include "Object.h"
#include <string>

class CScript : public CObject  
{
public:
	virtual void SetCamera();
	virtual void Update();
	CScript(const std::string& name);
	virtual ~CScript();

	virtual std::string GetMapName();
	virtual std::string GetModName();

	bool wantCameraControl;
	bool onlySinglePlayer;
	bool loadGame;
};

#endif /* SCRIPT_H */
