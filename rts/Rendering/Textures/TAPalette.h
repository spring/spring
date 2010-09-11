/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TAPALETTE_H
#define TAPALETTE_H

class CTAPalette
{
public:
	CTAPalette();
	~CTAPalette();

	inline unsigned char* operator[] (int a){
		return p[a];
	}

	unsigned char p[256][4];

	int NumTeamColors() const { return 10; }
	void Init(void);
};
extern CTAPalette palette;

#endif /* TAPALETTE_H */
