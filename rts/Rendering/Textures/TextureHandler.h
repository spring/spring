#ifndef TEXTUREHANDLER_H
#define TEXTUREHANDLER_H
// TextureHandler.h: interface for the CTextureHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <map>
#include <string>
#include <vector>
#include "Rendering/GL/myGL.h"
#include "Rendering/UnitModels/3DModelParser.h"

struct TexFile;
class CFileHandler;

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
		GLuint tex1;
		unsigned int tex1SizeX;
		unsigned int tex1SizeY;
		GLuint tex2;
		unsigned int tex2SizeX;
		unsigned int tex2SizeY;
	};

	CTextureHandler();
	virtual ~CTextureHandler();

	void SetTATexture();
	UnitTexture* GetTATexture(std::string name, int team, int teamTex);
	UnitTexture* GetTATexture(const std::string& name);

	std::vector<S3DOModel *> loadTextures;
	void Update();
	void LoadS3OTexture(S3DOModel *model);
	int LoadS3OTextureNow(const std::string& tex1, const std::string& tex2);
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
	const std::map<std::string, UnitTexture*>& GetGlobalTextures() const { return textures; }

private:
	std::map<std::string, UnitTexture*> textures;
	GLuint globalTex;
	int bigTexX;
	int bigTexY;

	std::map<std::string, int> s3oTextureNames;
	std::vector<S3oTex> s3oTextures;

	TexFile* CreateTeamTex(const std::string& name, const std::string& name2, int team);
};

extern CTextureHandler* texturehandler;

#endif /* TEXTUREHANDLER_H */
