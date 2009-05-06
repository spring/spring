#ifndef __GAME_CONTROLLER_H__
#define __GAME_CONTROLLER_H__

#include <string>

class CGameController
{
public:
	CGameController(void);
	virtual ~CGameController(void);
	virtual bool Draw(void);
	virtual bool Update(void);
	virtual int KeyPressed(unsigned short k,bool isRepeat);
	virtual int KeyReleased(unsigned short k);
	virtual void ResizeEvent() { return; }

	bool userWriting; // true if user is writing
	int  writingPos;  // current writing position
	bool ignoreNextChar;
	char ignoreChar;
	std::string userInput;
	std::string userPrompt;

protected:
	void PasteClipboard();
};

extern CGameController* activeController;

#endif // __GAME_CONTROLLER_H__
