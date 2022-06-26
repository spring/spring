/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCANCODES_H
#define SCANCODES_H

#include "IKeys.h"

class CScanCodes : public IKeys {
public:

	bool IsModifier(int code);

	void Reset();
	void PrintNameToCode() const;
	void PrintCodeToName() const;
	std::string GetName(int code) const;
	std::string GetDefaultName(int code) const;

	static int GetNormalizedSymbol(int sym);
};

extern CScanCodes scanCodes;

#endif /* SCANCODES_H */
