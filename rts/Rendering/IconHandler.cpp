/* Author: Teake Nutma */

#include "StdAfx.h"
#include <algorithm>
#include <assert.h>
#include <locale>
#include <cctype>
#include <vector>
#include <string>
#include "GlobalStuff.h"
#include "LogOutput.h"
#include "IconHandler.h"
#include "Lua/LuaParser.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/FileSystem/FileHandler.h"
#include "mmgr.h"

using std::string;

CIconHandler* iconHandler;


/******************************************************************************/
//
//  CIconHandler
//

CIconHandler::CIconHandler()
{
	PrintLoadMsg("Parsing unit icons");

	defTexID = 0;

	LoadIcons("gamedata/icontypes.lua");

	IconMap::iterator it = iconMap.find("default");
	if (it != iconMap.end()) {
		defIconData = it->second;
	}
	else {
		defIconData = SAFE_NEW CIconData("default", GetDefaultTexture(),
																		 1.0f, 1.0f, false, false);
		defIconData->Ref();
		iconMap["default"] = defIconData;
	}
}



CIconHandler::~CIconHandler()
{
	IconMap::iterator it;
	for (it = iconMap.begin(); it != iconMap.end(); ++it) {
		CIconData* iconData = it->second;
		iconData->UnRef();
	}
	glDeleteTextures(1, &defTexID);
}


bool CIconHandler::AddIcon(const string& iconName, const string& textureName,
                           float size, float distance, bool radAdj)
{
	unsigned int texID;

	bool ownTexture = true;

	try {
		CBitmap bitmap;
		if (!textureName.empty() && bitmap.Load(textureName)) {
			texID = bitmap.CreateTexture(true);
			glBindTexture(GL_TEXTURE_2D, texID);
			if (GLEW_EXT_texture_edge_clamp) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			} else {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			}
		} else {
			texID = GetDefaultTexture();
			ownTexture = false;
		}
	}
	catch (const content_error&) {
		// Ignore non-existant file.
		return false;
	}

	IconMap::iterator it = iconMap.find(iconName);
	if (it != iconMap.end()) {
		FreeIcon(iconName);
	}

	CIconData* iconData =
		SAFE_NEW CIconData(iconName, texID,  size, distance, radAdj, ownTexture);
	iconData->Ref();
	iconMap[iconName] = iconData;

	if (iconName == "default") {
		defIconData = iconData;
	}

	return true;
}


bool CIconHandler::FreeIcon(const string& iconName)
{
	IconMap::iterator it = iconMap.find(iconName);
	if (it == iconMap.end()) {
		return false;
	}

	CIconData* iconData = it->second;
	iconData->CopyData(defIconData);
	iconData->UnRef();

	iconMap.erase(iconName);

	return true;
}


bool CIconHandler::LoadIcons(const string& filename)
{
	LuaParser luaParser(filename, SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
	if (!luaParser.Execute()) {
		logOutput.Print("%s: %s",
		                filename.c_str(), luaParser.GetErrorLog().c_str());
	}

	const LuaTable iconTypes = luaParser.GetRoot();

	std::vector<string> iconNames;
	iconTypes.GetKeys(iconNames);

	for (int i = 0; i < iconNames.size(); i++) {
		const string& iconName = iconNames[i];
		const LuaTable iconTable = iconTypes.SubTable(iconName);
		const string texName = iconTable.GetString("bitmap",  "");
		const float size     = iconTable.GetFloat("size",     1.0f);
		const float dist     = iconTable.GetFloat("distance", 1.0f);
		const bool radiusAdjust = iconTable.GetBool("radiusAdjust", false);
		AddIcon(iconName, texName, size, dist, radiusAdjust);
	}

	return true;
}


CIcon CIconHandler::GetIcon(const string& iconName) const
{
	IconMap::const_iterator it = iconMap.find(iconName);
	if (it == iconMap.end()) {
		return CIcon(const_cast<CIconData*>(defIconData));
	} else {
		return CIcon(it->second);
	}
}


unsigned int CIconHandler::GetDefaultTexture()
{
	// FIXME: just use a PNG ?

	if (defTexID != 0) {
		return defTexID;
	}

	unsigned char si[128 * 128 * 4];
	for (int y = 0; y < 128; ++y) {
		for (int x = 0; x < 128; ++x) {
			const int index = ((y * 128) + x) * 4;
			const int dx = (x - 64);
			const int dy = (y - 64);
			const float r = sqrtf((dx * dx) + (dy * dy)) / 64.0f;
			if (r > 1.0f) {
				si[index + 0] = 0;
				si[index + 1] = 0;
				si[index + 2] = 0;
				si[index + 3] = 0;
			} else {
				const unsigned char val = (255 - (r * r * r * 255));
				si[index + 0] = val;
				si[index + 1] = val;
				si[index + 2] = val;
				si[index + 3] = 255;
			}
		}
	}

	CBitmap bitmap(si, 128, 128);
	defTexID = bitmap.CreateTexture(false);

	glBindTexture(GL_TEXTURE_2D, defTexID);
	if (GLEW_EXT_texture_edge_clamp) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	return defTexID;
}


/******************************************************************************/
//
//  CIcon
//

CIcon::CIcon()
{
	data = const_cast<CIconData*>(iconHandler->GetDefaultIconData());
	data->Ref();
}


CIcon::CIcon(CIconData* d)
{
	data = d;
	data->Ref();
}


CIcon::CIcon(const CIcon& icon)
{
	data = icon.data;
	data->Ref();
}


CIcon& CIcon::operator=(const CIcon& icon)
{
	if (data != icon.data) {
		data->UnRef();
		data = icon.data;
		data->Ref();
	}
	return *this;
}


CIcon::~CIcon()
{
	data->UnRef();
}


/******************************************************************************/
//
//  CIconData
//

//CIconData::CIconData() FIXME
//: ownTexture(true), refCount(0), texID(0)
//{
//}


CIconData::CIconData(const std::string& _name, unsigned int _texID,
                     float _size, float _distance, bool radAdj, bool ownTex)
: ownTexture(ownTex), refCount(0),
  name(_name), texID(_texID),
  size(_size), distance(_distance),
  radiusAdjust(radAdj)
{
	distSqr = distance * distance;
}


CIconData::~CIconData()
{
	if (ownTexture) {
		glDeleteTextures(1, &texID);
	}
}


void CIconData::Ref()
{
	refCount++;
}


void CIconData::UnRef()
{
	refCount--;
	if (refCount <= 0) {
		delete this;
	}
}


void CIconData::CopyData(const CIconData* iconData)
{
	name         = iconData->name;
	texID        = iconData->texID;
	size         = iconData->size;
	distance     = iconData->distance;
	distSqr      = iconData->distSqr;
	radiusAdjust = iconData->radiusAdjust;
	ownTexture   = false;
}


void CIconData::BindTexture() const
{
	glBindTexture(GL_TEXTURE_2D, texID);
}


void CIconData::Draw(float x0, float y0, float x1, float y1) const
{
	glBindTexture(GL_TEXTURE_2D, texID);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(x0, y0);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(x1, y0);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(x1, y1);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(x0, y1);
	glEnd();
}


void CIconData::Draw(const float3& botLeft, const float3& botRight,
                     const float3& topLeft, const float3& topRight) const
{
	glBindTexture(GL_TEXTURE_2D, texID);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f); glVertexf3(botLeft);
	glTexCoord2f(1.0f, 1.0f); glVertexf3(botRight);
	glTexCoord2f(1.0f, 0.0f); glVertexf3(topRight);
	glTexCoord2f(0.0f, 0.0f); glVertexf3(topLeft);
	glEnd();
}


/******************************************************************************/
