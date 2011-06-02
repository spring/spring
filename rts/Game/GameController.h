/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_CONTROLLER_H_
#define _GAME_CONTROLLER_H_

#include <string>

class CGameController
{
public:
	CGameController();
	virtual ~CGameController();

	virtual bool Draw();
	virtual bool Update();
	virtual int KeyPressed(unsigned short key, bool isRepeat);
	virtual int KeyReleased(unsigned short key);
	virtual void ResizeEvent() {}

	/// true if user is writing
	bool userWriting;
	/// current writing position
	int  writingPos;
	bool ignoreNextChar;
	char ignoreChar;
	std::string userInput;
	std::string userPrompt;

	void PasteClipboard();
};

extern CGameController* activeController;

#endif // _GAME_CONTROLLER_H_
