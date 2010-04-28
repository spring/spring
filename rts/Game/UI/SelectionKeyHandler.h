/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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

	void LoadSelectionKeys();

	std::string ReadToken(std::string& s);
	std::string ReadDelimiter(std::string& s);

	void DoSelection(std::string selectString);

private:
	int selectNumber;	//used to go through all possible units when selecting only a few
};

extern CSelectionKeyHandler *selectionKeys;

#endif /* SELECTIONKEYHANDLER_H */
