/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TA_PALETTE_H
#define TA_PALETTE_H

#include <cstdint>
#include "System/Color.h"

class CFileHandler;
class CTAPalette {
public:
	CTAPalette() {
		for (SColor& color: colors) {
			color.r = 0;
			color.g = 0;
			color.b = 0;
		}
	}

	void Init(CFileHandler&);

	const uint8_t* operator[] (unsigned int a) const { return colors[a]; }

public:
	static constexpr unsigned int NUM_PALETTE_ENTRIES = 256;

private:
	SColor colors[NUM_PALETTE_ENTRIES];
};

#endif /* TA_PALETTE_H */
