/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SELECTION_KEY_HANDLER_H
#define SELECTION_KEY_HANDLER_H

#include "InputReceiver.h"
#include <vector>

class CSelectionKeyHandler : public CInputReceiver
{
public:
	CSelectionKeyHandler(): selectNumber(0) {}

	void DoSelection(std::string selectString);

private:
	/**
	 * Removes and returns the first part of the string.
	 * Using the first of the encountered delimitters: '_', '+', end-of-string
	 */
	static std::string ReadToken(std::string& str);
	/**
	 * Removes and returns a delimiter (the first char of the string),
	 * or the empty string, if str is empty.
	 */
	static std::string ReadDelimiter(std::string& str);

	/// used to go through all possible units when selecting only a few
	int selectNumber;
};

extern CSelectionKeyHandler* selectionKeys;

#endif /* SELECTION_KEY_HANDLER_H */
