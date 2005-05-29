#ifndef TAPALETTE_H
#define TAPALETTE_H
// TAPalette.h: interface for the CTAPalette class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TAPALETTE_H__54041764_94B6_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_TAPALETTE_H__54041764_94B6_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

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
#endif // !defined(AFX_TAPALETTE_H__54041764_94B6_11D4_AD55_0080ADA84DE3__INCLUDED_)


#endif /* TAPALETTE_H */
