/* Author: Teake Nutma */

#include "StdAfx.h"
#include "GlobalStuff.h"
#include "LogOutput.h"
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
			float size=atof(tdfparser.SGetValueDef("1", "icontypes\\" + *it + "\\size").c_str());
			float distance=atof(tdfparser.SGetValueDef("1", "icontypes\\" + *it + "\\distance").c_str());
			bool radiusAdjust=!!atoi(tdfparser.SGetValueDef("0", "icontypes\\" + *it + "\\radiusadjust").c_str());
			// If we can't load the bitmap replace it with the default one.
			std::string bitmapLocation=tdfparser.SGetValueDef("", "icontypes\\" + *it + "\\bitmap");
			unsigned int texture;
			if(bitmap.Load(bitmapLocation)){
				texture = bitmap.CreateTexture(false);
			} else {
				texture = *GetStandardTexture();
			}
			icons[*it]=new CIcon(texture,size,distance,radiusAdjust);
		}

	} catch (const TdfParser::parse_error& e) {
		// Show parse errors in the infolog.
		logOutput.Print("%s:%d: %s", e.get_filename().c_str(), e.get_line(), e.what());
	} catch (const content_error& e) {
		// Ignore non-existant file.
	}

	// If the default icon doesn't exist we'll have to create one (as unitdef->iconType defaults to "default").
	std::map<std::string, CIcon*>::const_iterator it=icons.find("default");
	if(it==icons.end()){
		icons["default"]=new CIcon(*GetStandardTexture(),1,1,false);
	}
}

CIconHandler::~CIconHandler()
{
}

CIcon * CIconHandler::GetIcon(const std::string& iconName)
{
	std::map<std::string, CIcon*>::const_iterator it=icons.find(iconName);
	if(it==icons.end()){
		return icons["default"];
	} else {
		return it->second;
	}
}
float CIconHandler::GetDistance(const std::string& iconName)
{
	std::map<std::string, CIcon*>::const_iterator it=icons.find(iconName);
	if(it==icons.end()){
		return 1;
	} else {
		return it->second->distance;
	}
}

unsigned int *CIconHandler::GetStandardTexture()
{
	if(!standardTextureGenerated){
		unsigned char si[128*128*4];
		for(int y=0;y<128;++y){
			for(int x=0;x<128;++x){
				float r=sqrtf((y-64)*(y-64)+(x-64)*(x-64))/64.0f;
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
		standardTexture=standardIcon.CreateTexture(false);
		standardTextureGenerated=true;
	}
	return &standardTexture;
}
