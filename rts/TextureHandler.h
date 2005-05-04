// TextureHandler.h: interface for the CTextureHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TEXTUREHANDLER_H__C6D286E1_997B_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_TEXTUREHANDLER_H__C6D286E1_997B_11D4_AD55_0080ADA84DE3__INCLUDED_

#pragma warning(disable:4786)

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <map>
#include <string>

using namespace std;

class CTextureHandler  
{
public:
	struct UnitTexture {
		float xstart;
		float xend;
		float ystart;
		float yend;
	};

	void SetTexture();
	UnitTexture* GetTexture(string name,int team,int teamTex);
	UnitTexture* GetTexture(string name);

	CTextureHandler();
	virtual ~CTextureHandler();

	map<string,UnitTexture*> textures;
	unsigned int globalTex;
	int bigTexX;
	int bigTexY;
	bool newTexFound;
};

extern CTextureHandler* texturehandler;

#endif // !defined(AFX_TEXTUREHANDLER_H__C6D286E1_997B_11D4_AD55_0080ADA84DE3__INCLUDED_)

