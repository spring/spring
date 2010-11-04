/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TA_PALETTE_H
#define TA_PALETTE_H

class CTAPalette
{
public:
	CTAPalette();
	~CTAPalette();

	inline unsigned char* operator[] (int a) {
		return p[a];
	}

	int NumTeamColors() const { return 10; }
	void Init();

	unsigned char p[256][4];
};

extern CTAPalette palette;

#endif /* TA_PALETTE_H */
