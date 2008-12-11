#ifndef _3DOTEXTUREHANDLER_H
#define _3DOTEXTUREHANDLER_H
// _3DOTextureHandler.h: interface for the C3DOTextureHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <map>
#include <string>
#include <vector>
#include "Rendering/GL/myGL.h"
#include "Rendering/UnitModels/3DModel.h"

struct TexFile;
class CFileHandler;

class C3DOTextureHandler
{
public:
	struct UnitTexture {
		float xstart;
		float xend;
		float ystart;
		float yend;
	};

	C3DOTextureHandler();
	~C3DOTextureHandler();

	void Set3doAtlases() const;

	UnitTexture* Get3DOTexture(std::string name);

	unsigned int GetAtlasTex1ID() const  { return atlas3do1; }
	unsigned int GetAtlasTex2ID() const  { return atlas3do2; }
	unsigned int GetAtlasTexSizeX() const { return bigTexX; }
	unsigned int GetAtlasTexSizeY() const { return bigTexY; }
	const std::map<std::string, UnitTexture*>& GetAtlasTextures() const { return textures; }

private:
	std::map<std::string, UnitTexture*> textures;
	GLuint atlas3do1;
	GLuint atlas3do2;
	int bigTexX;
	int bigTexY;

	TexFile* CreateTex(const std::string& name, const std::string& name2, bool teamcolor = false);
};

extern C3DOTextureHandler* texturehandler3DO;

#endif /* _3DOTEXTUREHANDLER_H */
