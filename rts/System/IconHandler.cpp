/* Author: Teake Nutma */

#include "StdAfx.h"
#include "GlobalStuff.h"
#include "Game/UI/InfoConsole.h"
#include "IconHandler.h"
#include "TdfParser.h"
#include "Rendering/Textures/Bitmap.h"
#include "mmgr.h"
#include <algorithm>
#include <assert.h>
#include <locale>
#include <cctype>

CIconHandler* iconHandler;

CIconHandler::CIconHandler()
{
	standardTextureGenerated=false;

	try {

		PrintLoadMsg("Parsing unit icons");
		TdfParser tdfparser("gamedata/icontypes.tdf");
		std::vector<std::string> iconList = tdfparser.GetSectionList("icontypes");
		CBitmap bitmap;

		for (std::vector<std::string>::const_iterator it = iconList.begin(); it != iconList.end(); ++it) {
			//Parse the bitmap location, the size, and the unit radius adjustment.
			iconSizes[*it]=atof(tdfparser.SGetValueDef("1", "icontypes\\" + *it + "\\size").c_str());
			iconDistances[*it]=atof(tdfparser.SGetValueDef("1", "icontypes\\" + *it + "\\distance").c_str());
			iconLocations[*it]=tdfparser.SGetValueDef("", "icontypes\\" + *it + "\\bitmap");
			iconRadiusAdjust[*it]=!!atoi(tdfparser.SGetValueDef("0", "icontypes\\" + *it + "\\radiusadjust").c_str());
			// If we can't load the bitmap replace it with the default one.
			if(bitmap.Load(iconLocations[*it])){
				bitmap.CreateAlpha(0,0,0);
				iconTextures[*it] = bitmap.CreateTexture(false);
			} else {
				iconTextures[*it] = *GetStandardTexture();
			}
		}

	} catch (const TdfParser::parse_error& e) {
		// Show parse errors in the infolog.
		info->AddLine("%s:%d: %s", e.get_filename().c_str(), e.get_line(), e.what());
	} catch (const content_error& e) {
		// Ignore non-existant file.
	}

	// If the default icon doesn't exist we'll have to create one (as unitdef->iconType defaults to "default").
	std::map<std::string, std::string>::const_iterator it=iconLocations.find("default");
	if(it==iconLocations.end()){
		iconTextures["default"] = *GetStandardTexture();
		iconSizes["default"] = 1;
		iconDistances["default"] = 1;
		iconRadiusAdjust["default"] = false;
	}
}

CIconHandler::~CIconHandler()
{
}

unsigned int *CIconHandler::GetIcon(const std::string& iconName)
{
	std::map<std::string, unsigned int>::const_iterator it=iconTextures.find(iconName);
	if(it==iconTextures.end()){
		return GetStandardTexture();
	} else {
		return &iconTextures[iconName];
	}
}

float CIconHandler::GetSize(const std::string& iconName)
{
	std::map<std::string, float>::const_iterator it=iconSizes.find(iconName);
	if(it==iconSizes.end()){
		return 1;
	} else {
		return it->second;
	}
}
bool CIconHandler::GetRadiusAdjust(const std::string& iconName)
{
	std::map<std::string, bool>::const_iterator it=iconRadiusAdjust.find(iconName);
	if(it==iconRadiusAdjust.end()){
		return false;
	} else {
		return it->second;
	}
}
float CIconHandler::GetDistance(const std::string& iconName)
{
	std::map<std::string, float>::const_iterator it=iconDistances.find(iconName);
	if(it==iconDistances.end()){
		return 1;
	} else {
		return it->second;
	}
}

unsigned int *CIconHandler::GetStandardTexture()
{
	if(!standardTextureGenerated){
		unsigned char si[128*128*4];
		for(int y=0;y<128;++y){
			for(int x=0;x<128;++x){
				float r=sqrtf((y-64)*(y-64)+(x-64)*(x-64))/64.0;
				if(r>1){
					si[(y*128+x)*4+0]=0;
					si[(y*128+x)*4+1]=0;
					si[(y*128+x)*4+2]=0;
					si[(y*128+x)*4+3]=0;
				} else {
					si[(y*128+x)*4+0]=(unsigned char)(255-r*r*r*255);
					si[(y*128+x)*4+1]=(unsigned char)(255-r*r*r*255);
					si[(y*128+x)*4+2]=(unsigned char)(255-r*r*r*255);
					si[(y*128+x)*4+3]=255;
				}
			}
		}
		CBitmap standardIcon(si,128,128);
		standardTexture=standardIcon.CreateTexture(true);
		standardTextureGenerated=true;
	}
	return &standardTexture;
}
