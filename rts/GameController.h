#pragma once
#include <string>

class CGameController
{
public:
	CGameController(void);
	virtual ~CGameController(void);
	virtual bool Draw(void);
	virtual bool Update(void);
	virtual int KeyPressed(unsigned char k,bool isRepeat);
	virtual int KeyReleased(unsigned char k);

	bool userWriting;						//true if user is writing
	bool ignoreNextChar;
	char ignoreChar;
	std::basic_string<char> userInput;
	std::basic_string<char> userPrompt;

};

extern CGameController* activeController;