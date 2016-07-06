/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IconHandler.h"

#include <algorithm>
#include <assert.h>
#include <locale>
#include <cctype>
#include <cmath>

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "System/Log/ILog.h"
#include "Lua/LuaParser.h"
#include "Textures/Bitmap.h"
#include "System/Exceptions.h"

namespace icon {
using std::string;

CIconHandler* iconHandler = NULL;
CIconData CIconHandler::safetyData;


/******************************************************************************/
//
//  CIconHandler
//

CIconHandler::CIconHandler()
{
	defTexID = 0;
	defIconData = NULL;

	LoadIcons("gamedata/icontypes.lua");

	IconMap::iterator it = iconMap.find("default");
	if (it != iconMap.end()) {
		defIconData = it->second.data;
	}
	else {
		defIconData = new CIconData("default", GetDefaultTexture(), 1.0f, 1.0f, false, false, 128, 128);
		iconMap["default"] = CIcon(defIconData);
	}
}


CIconHandler::~CIconHandler()
{
	glDeleteTextures(1, &defTexID);
}


bool CIconHandler::LoadIcons(const string& filename)
{
	LuaParser luaParser(filename, SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
	if (!luaParser.Execute()) {
		LOG_L(L_WARNING, "%s: %s",
				filename.c_str(), luaParser.GetErrorLog().c_str());
	}

	const LuaTable iconTypes = luaParser.GetRoot();

	std::vector<string> iconNames;
	iconTypes.GetKeys(iconNames);

	for (size_t i = 0; i < iconNames.size(); i++) {
		const string& iconName = iconNames[i];
		const LuaTable iconTable = iconTypes.SubTable(iconName);
		AddIcon(
			iconName,
			iconTable.GetString("bitmap",       ""),
			iconTable.GetFloat ("size",         1.0f),
			iconTable.GetFloat ("distance",     1.0f),
			iconTable.GetBool  ("radiusAdjust", false)
		);
	}

	return true;
}


bool CIconHandler::AddIcon(const string& iconName, const string& textureName,
                           float size, float distance, bool radAdj)
{
	unsigned int texID;
	int xsize, ysize;

	bool ownTexture = true;

	try {
		CBitmap bitmap;
		if (!textureName.empty() && bitmap.Load(textureName)) {
			texID = bitmap.CreateMipMapTexture();
			
			glBindTexture(GL_TEXTURE_2D, texID);
			const GLenum wrapMode = GLEW_EXT_texture_edge_clamp ?
			                        GL_CLAMP_TO_EDGE : GL_CLAMP;
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
			xsize = bitmap.xsize;
			ysize = bitmap.ysize;
		} else {
			texID = GetDefaultTexture();
			xsize = 128;
			ysize = 128;
			ownTexture = false;
		}
	} catch (const content_error& ex) {
		// bail on non-existant file
		LOG_L(L_DEBUG, "Failed to add icon: %s", ex.what());
		return false;
	}

	IconMap::iterator it = iconMap.find(iconName);
	if (it != iconMap.end()) {
		FreeIcon(iconName);
	}

	CIconData* iconData = new CIconData(iconName, texID,  size, distance,
	                                         radAdj, ownTexture, xsize, ysize);

	iconMap[iconName] = CIcon(iconData);

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
	if (iconName == "default") {
		return false;
	}

	CIconData* iconData = it->second.data;
	iconData->CopyData(defIconData);

	iconMap.erase(iconName);

	return true;
}


CIcon CIconHandler::GetIcon(const string& iconName) const
{
	IconMap::const_iterator it = iconMap.find(iconName);
	if (it == iconMap.end()) {
		return CIcon(defIconData);
	} else {
		return it->second;
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
			const float r = std::sqrt((dx * dx) + (dy * dy)) / 64.0f;
			if (r > 1.0f) {
				si[index + 0] = 0;
				si[index + 1] = 0;
				si[index + 2] = 0;
				si[index + 3] = 0;
			} else {
				const unsigned char val = (unsigned char)(255 - (r * r * r * 255));
				si[index + 0] = val;
				si[index + 1] = val;
				si[index + 2] = val;
				si[index + 3] = 255;
			}
		}
	}

	CBitmap bitmap(si, 128, 128);
	defTexID = bitmap.CreateTexture();

	glBindTexture(GL_TEXTURE_2D, defTexID);

	const GLenum wrapMode = GLEW_EXT_texture_edge_clamp ?
													GL_CLAMP_TO_EDGE : GL_CLAMP;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

	return defTexID;
}


/******************************************************************************/
//
//  CIcon
//

CIcon::CIcon() 
{
	if (iconHandler && iconHandler->defIconData) {
		data = iconHandler->defIconData;
	} else {
		data = &CIconHandler::safetyData;
	}
	data->Ref();
}


CIcon::CIcon(CIconData* d)
{
	data = d ? d : &CIconHandler::safetyData;
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
	data = nullptr;
}





/******************************************************************************/
//
//  CIconData
//

CIconData::CIconData() :
	ownTexture(false),
	refCount(123456),
	name("safety"),
	texID(0),
	xsize(1),
	ysize(1),
	size(1.0f),
	distance(1.0f),
	distSqr(1.0f),
	radiusAdjust(false)
{
}

CIconData::CIconData(const std::string& _name, unsigned int _texID,
                     float _size, float _distance, bool radAdj, bool ownTex,
                     int _xsize, int _ysize)
: ownTexture(ownTex), refCount(0),
  name(_name), texID(_texID),
  xsize(_xsize), ysize(_ysize),
  size(_size), distance(_distance),
  radiusAdjust(radAdj)
{
	distSqr = distance * distance;
}

CIconData::~CIconData()
{
	if (ownTexture) {
		glDeleteTextures(1, &texID);
		texID = 0;
	}
}


void CIconData::Ref()
{
	refCount++;
}

void CIconData::UnRef()
{
	if ((--refCount) <= 0) {
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
	xsize        = iconData->xsize;
	ysize        = iconData->ysize;
	ownTexture   = false;
}

void CIconData::BindTexture() const
{
	glBindTexture(GL_TEXTURE_2D, texID);
}

void CIconData::DrawArray(CVertexArray* va, float x0, float y0, float x1, float y1, const unsigned char* c) const
{
	va->AddVertex2dTC(x0, y0, 0.0f, 0.0f, c);
	va->AddVertex2dTC(x1, y0, 1.0f, 0.0f, c);
	va->AddVertex2dTC(x1, y1, 1.0f, 1.0f, c);
	va->AddVertex2dTC(x0, y1, 0.0f, 1.0f, c);
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

}
