#ifndef TEXTUREHANDLER_H
#define TEXTUREHANDLER_H
// TextureHandler.h: interface for the CTextureHandler class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

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
	struct S3oTex {
		int num;
		unsigned int tex1;
		unsigned int tex1SizeX;
		unsigned int tex1SizeY;
		unsigned int tex2;
		unsigned int tex2SizeX;
		unsigned int tex2SizeY;
	};

	CTextureHandler();
	virtual ~CTextureHandler();

	void SetTATexture();
	UnitTexture* GetTATexture(string name,int team,int teamTex);
	UnitTexture* GetTATexture(string name);

	int LoadS3OTexture(string tex1, string tex2);
	void SetS3oTexture(int num);

	const S3oTex* GetS3oTex(int num) {
		if ((num < 0) || (num >= (int)s3oTextures.size())) {
			return NULL;
		}
		return &s3oTextures[num];
	}

	unsigned int GetGlobalTexID() const  { return globalTex; }
	unsigned int GetGlobalTexSizeX() const { return bigTexX; }
	unsigned int GetGlobalTexSizeY() const { return bigTexY; }
	const map<string, UnitTexture*>& GetGlobalTextures() const { return textures; }

private:
	map<string,UnitTexture*> textures;
	unsigned int globalTex;
	int bigTexX;
	int bigTexY;

	map<string,int> s3oTextureNames;
	vector<S3oTex> s3oTextures;

	string GetLine(CFileHandler& fh);
	TexFile* CreateTeamTex(string name, string name2,int team);
};

extern CTextureHandler* texturehandler;

#endif /* TEXTUREHANDLER_H */
