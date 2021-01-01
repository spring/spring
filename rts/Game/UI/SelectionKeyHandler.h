/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SELECTION_KEY_HANDLER_H
#define SELECTION_KEY_HANDLER_H

#include <vector>

#include "InputReceiver.h"

class CUnit;
class CSelectionKeyHandler : public CInputReceiver {
public:
	void Init() {
		numDoSelects = 0;
		selectNumber = 0;
	}
	void Kill() { selection.clear(); }
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

private:
	int numDoSelects = 0;
	/// used to go through all possible units when selecting only a few
	int selectNumber = 0;

	std::vector<CUnit*> selection;
};

extern CSelectionKeyHandler selectionKeys;

#endif /* SELECTION_KEY_HANDLER_H */
