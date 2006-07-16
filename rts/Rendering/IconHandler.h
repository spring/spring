/* Author: Teake Nutma */

#ifndef ICONHANDLER_H
#define ICONHANDLER_H

#include <map>
#include <string>
#include <vector>

class CIcon
{
public:
	unsigned int texture;
	float size;
	float distance;
	bool radiusAdjust;
	CIcon(unsigned int tex, float siz, float dis, bool radius)
	{
		texture		= tex;
		size		= siz;
		distance	= dis;
		radiusAdjust= radius;
	};
};

class CIconHandler
{
public:
	CIconHandler(void);
	~CIconHandler(void);
	CIcon * GetIcon(const std::string& iconName);
	float GetDistance(const std::string& iconName);
private:
	unsigned int *GetStandardTexture();
	unsigned int standardTexture;
	bool standardTextureGenerated;
	std::map<std::string, CIcon*> icons;
};

extern CIconHandler* iconHandler;

#endif
