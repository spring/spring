#ifndef SELECTIONKEYHANDLER_H
#define SELECTIONKEYHANDLER_H
/*pragma once removed*/
#include "InputReceiver.h"
#include <vector>

using namespace std;

class CSelectionKeyHandler :
	public CInputReceiver
{
public:
	CSelectionKeyHandler(void);
	~CSelectionKeyHandler(void);
	bool KeyPressed(unsigned short key);
	bool KeyReleased(unsigned short key);
	string ReadToken(string& s);
	string ReadDelimiter(string& s);

	struct HotKey {
		unsigned char key;
		bool shift;
		bool control;
		bool alt;

		string select;
	};
	vector<HotKey> hotkeys;
	void DoSelection(string selectString);

	int selectNumber;	//used to go through all possible units when selecting only a few
};



#endif /* SELECTIONKEYHANDLER_H */
