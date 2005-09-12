#ifndef TEXTUREHANDLER_H
#define TEXTUREHANDLER_H
// TextureHandler.h: interface for the CTextureHandler class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

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

#endif /* TEXTUREHANDLER_H */
