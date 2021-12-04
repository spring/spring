/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TA_PALETTE_H
#define TA_PALETTE_H

class CFileHandler;
class CTAPalette {
public:
	enum { NUM_PALETTE_ENTRIES = 256 };

	CTAPalette();

	inline const unsigned char* operator[] (int a) const { return p[a]; }

	void Init(CFileHandler&);

private:
	unsigned char p[NUM_PALETTE_ENTRIES][4];
};

extern CTAPalette palette;

#endif /* TA_PALETTE_H */
