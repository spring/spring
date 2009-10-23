#ifndef SCRIPT_H
#define SCRIPT_H
// Script.h: interface for the CScript class.
//
//////////////////////////////////////////////////////////////////////

#include <string>

class CScript
{
public:
	virtual void SetCamera();
	virtual void Update();
	virtual void GotChatMsg(const std::string& msg, int player);
	CScript(const std::string& name);
	virtual ~CScript();

	virtual void ScriptSelected();
	virtual void GameStart();

	bool wantCameraControl;
	bool onlySinglePlayer;
	bool loadGame;
};

#endif /* SCRIPT_H */
