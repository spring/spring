#ifndef TAPALETTE_H
#define TAPALETTE_H
// TAPalette.h: interface for the CTAPalette class.
//
//////////////////////////////////////////////////////////////////////

class CTAPalette  
{
public:
	CTAPalette();
	virtual ~CTAPalette();

/*	const_reference operator[](size_type _P) const
		{return (*(begin() + _P)); }
*/
	inline unsigned char* operator[] (int a){
		return p[a];
	}

	unsigned char p[256][4];

	unsigned char teamColor[10][4];
	void Init(void);
};
extern CTAPalette palette;

#endif /* TAPALETTE_H */
