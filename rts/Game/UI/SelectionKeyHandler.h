#ifndef SELECTIONKEYHANDLER_H
#define SELECTIONKEYHANDLER_H

#include "InputReceiver.h"
#include <vector>

class CSelectionKeyHandler :
	public CInputReceiver
{
public:
	CSelectionKeyHandler(void);
	~CSelectionKeyHandler(void);
	bool KeyPressed(unsigned short key, bool isRepeat);
	bool KeyReleased(unsigned short key);
	std::string ReadToken(std::string& s);
	std::string ReadDelimiter(std::string& s);

	struct HotKey {
		unsigned char key;
		bool shift;
		bool control;
		bool alt;

		std::string select;
	};
	std::vector<HotKey> hotkeys;
	void DoSelection(std::string selectString);

	int selectNumber;	//used to go through all possible units when selecting only a few
};

extern CSelectionKeyHandler *selectionKeys;

#endif /* SELECTIONKEYHANDLER_H */
