#ifndef TAPALETTE_H
#define TAPALETTE_H
// TAPalette.h: interface for the CTAPalette class.
//
//////////////////////////////////////////////////////////////////////

class CTAPalette
{
public:
	CTAPalette();
	~CTAPalette();

	inline unsigned char* operator[] (int a){
		return p[a];
	}

	unsigned char p[256][4];

	int NumTeamColors() { return 10; }
	void Init(void);
};
extern CTAPalette palette;

#endif /* TAPALETTE_H */
