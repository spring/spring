/* Author: Teake Nutma */

#ifndef ICONHANDLER_H
#define ICONHANDLER_H

#include <map>
#include <string>
#include <vector>

class CIconHandler
{
public:
	CIconHandler(void);
	~CIconHandler(void);
	unsigned int *GetIcon(const std::string& iconName);
	float GetSize(const std::string& iconName);
	bool GetRadiusAdjust(const std::string& iconName);
private:
	unsigned int *GetStandardTexture();
	unsigned int standardTexture;
	bool standardTextureGenerated;
	std::map<std::string, std::string> iconLocations; // locations of the external bitmaps.
	std::map<std::string, unsigned int> iconTextures; // holds the textures.
	std::map<std::string, float> iconSizes; // holds the size multiplier.
	std::map<std::string, bool> iconRadiusAdjust; // adjust to unit radius or not.
};

extern CIconHandler* iconHandler;

#endif
