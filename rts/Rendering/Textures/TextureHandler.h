#ifndef TEXTUREHANDLER_H
#define TEXTUREHANDLER_H
// TextureHandler.h: interface for the CTextureHandler class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <map>
#include <string>
#include <vector>
struct TexFile;
class CFileHandler;

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

	CTextureHandler();
	virtual ~CTextureHandler();

	void SetTATexture();
	UnitTexture* GetTATexture(string name,int team,int teamTex);
	UnitTexture* GetTATexture(string name);

	int LoadS3OTexture(string tex1, string tex2);
	void SetS3oTexture(int num);

private:
	map<string,UnitTexture*> textures;
	unsigned int globalTex;
	int bigTexX;
	int bigTexY;

	struct S3oTex{
		int num;
		unsigned int tex1;
		unsigned int tex2;
	};
	map<string,int> s3oTextureNames;
	vector<S3oTex> s3oTextures;

	string GetLine(CFileHandler& fh);
	TexFile* CreateTeamTex(string name, string name2,int team);
};

extern CTextureHandler* texturehandler;

#endif /* TEXTUREHANDLER_H */
