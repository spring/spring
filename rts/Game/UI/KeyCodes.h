/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef KEYCODES_H
#define KEYCODES_H

#include "IKeys.h"

class CKeyCodes : public IKeys {
public:

	bool IsModifier(int code);

	void Reset();
	void PrintNameToCode() const;
	void PrintCodeToName() const;
	std::string GetName(int code) const;
	std::string GetDefaultName(int code) const;

	static int GetNormalizedSymbol(int sym);
};

extern CKeyCodes keyCodes;

#endif /* KEYCODES_H */
