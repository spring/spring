#ifndef SCRIPT_H
#define SCRIPT_H
// Script.h: interface for the CScript class.
//
//////////////////////////////////////////////////////////////////////

#include "System/Object.h"
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

#endif /* SCRIPT_H */
