// NamedTextures.cpp: implementation of the CNamedTextures class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include "bitops.h"
#include "NamedTextures.h"
#include "Rendering/GL/myGL.h"
#include "Bitmap.h"
#include "GlobalUnsynced.h"


map<string, CNamedTextures::TexInfo> CNamedTextures::texMap;


/******************************************************************************/

void CNamedTextures::Init()
{
}


void CNamedTextures::Kill()
{
	map<string, TexInfo>::iterator it;
	for (it = texMap.begin(); it != texMap.end(); ++it) {
		const GLuint texID = it->second.id;
		glDeleteTextures(1, &texID);
	}
}


/******************************************************************************/

bool CNamedTextures::Bind(const string& texName)
{
	if (texName.empty()) {
		return false;
	}

	map<string, TexInfo>::iterator it = texMap.find(texName);
	if (it != texMap.end()) {
		const GLuint texID = it->second.id;
		if (texID == 0) {
			glBindTexture(GL_TEXTURE_2D, 0);
			return false;
		} else {
			glBindTexture(GL_TEXTURE_2D, texID);
			return true;
		}
	}

	//! strip off the qualifiers
	string filename = texName;
	bool border  = false;
	bool clamped = false;
	bool nearest = false;
	bool linear  = false;
	bool aniso   = false;
	bool invert  = false;
	bool greyed  = false;
	bool tint    = false;
	float tintColor[3];

	if (filename[0] == ':') {
		int p;
		for (p = 1; p < (int)filename.size(); p++) {
			const char ch = filename[p];
			if (ch == ':')      { break; }
			else if (ch == 'n') { nearest = true; }
			else if (ch == 'l') { linear  = true; }
			else if (ch == 'a') { aniso   = true; }
			else if (ch == 'i') { invert  = true; }
			else if (ch == 'g') { greyed  = true; }
			else if (ch == 'c') { clamped = true; }
			else if (ch == 'b') { border  = true; }
			else if (ch == 't') {
				const char* cstr = filename.c_str() + p + 1;
				const char* start = cstr;
				char* endptr;
				tintColor[0] = (float)strtod(start, &endptr);
				if ((start != endptr) && (*endptr == ',')) {
					start = endptr + 1;
					tintColor[1] = (float)strtod(start, &endptr);
					if ((start != endptr) && (*endptr == ',')) {
						start = endptr + 1;
						tintColor[2] = (float)strtod(start, &endptr);
						if (start != endptr) {
							tint = true;
							p += (endptr - cstr);
						}
					}
				}
			}
		}
		if (p < (int)filename.size()) {
			filename = filename.substr(p + 1);
		} else {
			filename.clear();
		}
	}

	//! get the image
	CBitmap bitmap;
	TexInfo texInfo;
	GLuint texID = 0;

	if (!bitmap.Load(filename)) {
		texMap[texName] = texInfo;
		glBindTexture(GL_TEXTURE_2D, 0);
		return false;
	}

	if (bitmap.type == CBitmap::BitmapTypeDDS) {
		texID = bitmap.CreateDDSTexture();
	} else {
		if (invert) {
			bitmap.InvertColors();
		}
		if (greyed) {
			bitmap.GrayScale();
		}
		if (tint) {
			bitmap.Tint(tintColor);
		}

		//! make the texture
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);

		if (clamped) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}

		if (nearest || linear) {
			if (border) {
				GLfloat white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);
			}

			if (nearest) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			} else {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}

			//! Note: NPOTs + nearest filtering seems broken on ATIs
			if ( !(count_bits_set(bitmap.xsize)==1 && count_bits_set(bitmap.ysize)==1) &&
				(!GLEW_ARB_texture_non_power_of_two || (gu->atiHacks && nearest)) )
			{
				bitmap = bitmap.CreateRescaled(next_power_of_2(bitmap.xsize),next_power_of_2(bitmap.ysize));
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
						bitmap.xsize, bitmap.ysize, border ? 1 : 0,
						GL_RGBA, GL_UNSIGNED_BYTE, bitmap.mem);
		} else {
			//! MIPMAPPING (default)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			if ((count_bits_set(bitmap.xsize)==1 && count_bits_set(bitmap.ysize)==1) ||
				GLEW_ARB_texture_non_power_of_two)
			{
				glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, bitmap.xsize, bitmap.ysize,
							GL_RGBA, GL_UNSIGNED_BYTE, bitmap.mem);
			} else {
				gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, bitmap.xsize, bitmap.ysize,
								GL_RGBA, GL_UNSIGNED_BYTE, bitmap.mem);
			}
		}

		if (aniso && GLEW_EXT_texture_filter_anisotropic) {
			static GLfloat maxAniso = -1.0f;
			if (maxAniso == -1.0f) {
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
			}
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
		}
	}

	texInfo.id    = texID;
	texInfo.type  = bitmap.type;
	texInfo.xsize = bitmap.xsize;
	texInfo.ysize = bitmap.ysize;

	texMap[texName] = texInfo;

	return true;
}


bool CNamedTextures::Free(const string& texName)
{
	if (texName.empty()) {
		return false;
	}
	map<string, TexInfo>::iterator it = texMap.find(texName);
	if (it != texMap.end()) {
		const GLuint texID = it->second.id;
		glDeleteTextures(1, &texID);
		texMap.erase(it);
		return true;
	}
	return false;
}


const CNamedTextures::TexInfo* CNamedTextures::GetInfo(const string& texName)
{
	if (texName.empty()) {
		return false;
	}
	map<string, TexInfo>::const_iterator it = texMap.find(texName);
	if (it != texMap.end()) {
		return &it->second;
	}
	return NULL;
}


/******************************************************************************/
