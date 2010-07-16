/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SELECTION_KEY_HANDLER_H
#define SELECTION_KEY_HANDLER_H

#include "InputReceiver.h"
#include <vector>

class CSelectionKeyHandler : public CInputReceiver
{
public:
	CSelectionKeyHandler();
	~CSelectionKeyHandler();

	void LoadSelectionKeys();

	std::string ReadToken(std::string& s);
	std::string ReadDelimiter(std::string& s);

	void DoSelection(std::string selectString);

private:
	int selectNumber; ///< used to go through all possible units when selecting only a few
};

extern CSelectionKeyHandler* selectionKeys;

#endif /* SELECTION_KEY_HANDLER_H */
