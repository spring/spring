/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// must be included before streflop! else we get streflop/cmath resolve conflicts in its hash implementation files
#include <vector>
#include "NamedTextures.h"

#include "Rendering/GL/myGL.h"
#include "Bitmap.h"
#include "Rendering/GlobalRendering.h"
#include "System/bitops.h"
#include "System/type2.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"
#include "System/UnorderedMap.hpp"



namespace CNamedTextures {
	// maps names to texInfoVec indices
	static spring::unordered_map<std::string, size_t> texInfoMap;

	static std::vector<CNamedTextures::TexInfo> texInfoVec;
	static std::vector<size_t> freeIndices;
	static std::vector<std::string> waitingTextures;

	static spring::recursive_mutex mutex;

	/******************************************************************************/

	void Init()
	{
		texInfoMap.clear();
		texInfoMap.reserve(128);
		texInfoVec.clear();
		texInfoVec.reserve(128);

		freeIndices.clear();

		waitingTextures.clear();
		waitingTextures.reserve(16);
	}

	void Kill(bool shutdown)
	{
		decltype(texInfoMap) tempMap;

		const std::lock_guard<spring::recursive_mutex> lck(mutex);

		for (const auto& item: texInfoMap) {
			const size_t texIdx = item.second;
			const GLuint texID = texInfoVec[texIdx].id;

			if (shutdown || !texInfoVec[texIdx].persist) {
				glDeleteTextures(1, &texID);
				// always recycle non-persistent textures
				freeIndices.push_back(texIdx);
			} else {
				tempMap[item.first] = item.second;
			}
		}

		std::swap(texInfoMap, tempMap);
		waitingTextures.clear();
	}


	/******************************************************************************/

	static void InsertTex(const std::string& texName, const TexInfo& texInfo, bool loadTex)
	{
		// caller (GenInsertTex) already has lock
		if (!loadTex)
			waitingTextures.push_back(texName);

		if (freeIndices.empty()) {
			texInfoMap[texName] = texInfoVec.size();
			texInfoVec.push_back(texInfo);
		} else {
			// recycle
			texInfoMap[texName] = freeIndices.back();
			texInfoVec[freeIndices.back()] = texInfo;
			freeIndices.pop_back();
		}
	}

	static TexInfo GenTex(bool bindTex, bool persistTex)
	{
		GLuint texID = 0;
		glGenTextures(1, &texID);

		if (bindTex)
			glBindTexture(GL_TEXTURE_2D, texID);

		TexInfo texInfo;
		texInfo.id = texID;
		texInfo.persist = persistTex;
		return texInfo;
	}

	static void GenInsertTex(const std::string& texName, const TexInfo& texInfo, bool genTex, bool bindTex, bool loadTex, bool persistTex)
	{
		const std::lock_guard<spring::recursive_mutex> lck(mutex);

		if (!genTex) {
			InsertTex(texName, texInfo, loadTex);
			return;
		}

		InsertTex(texName, GenTex(bindTex, persistTex), loadTex);
	}

	static bool EraseTex(const std::string& texName)
	{
		const std::lock_guard<spring::recursive_mutex> lck(mutex);

		const auto it = texInfoMap.find(texName);

		if (it != texInfoMap.end()) {
			const size_t texIdx = it->second;
			const GLuint texID = texInfoVec[texIdx].id;

			glDeleteTextures(1, &texID);

			freeIndices.push_back(texIdx);
			texInfoMap.erase(it);
			return true;
		}

		return false;
	}



	static bool Load(const std::string& texName, unsigned int texID)
	{
		// strip off the qualifiers
		std::string filename = texName;
		bool border  = false;
		bool clamped = false;
		bool nearest = false;
		bool linear  = false;
		bool aniso   = false;
		bool invert  = false;
		bool greyed  = false;
		bool tint    = false;
		float tintColor[3];
		bool resize  = false;
		int2 resizeDimensions;

		if (filename[0] == ':') {
			size_t p;
			for (p = 1; p < filename.size(); p++) {
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
				else if (ch == 'r') {
					const char* cstr = filename.c_str() + p + 1;
					const char* start = cstr;
					char* endptr;
					resizeDimensions.x = (int)strtoul(start, &endptr, 10);
					if ((start != endptr) && (*endptr == ',')) {
						start = endptr + 1;
						resizeDimensions.y = (int)strtoul(start, &endptr, 10);
						if (start != endptr) {
							resize = true;
							p += (endptr - cstr);
						}
					}
				}
			}

			if (p < filename.size()) {
				filename = filename.substr(p + 1);
			} else {
				filename.clear();
			}
		}

		// get the image
		CBitmap bitmap;
		TexInfo texInfo;

		if (!bitmap.Load(filename)) {
			LOG_L(L_WARNING, "[NamedTextures::%s] could not load texture \"%s\"", __func__, filename.c_str());
			GenInsertTex(texName, texInfo, false, false, true, false);
			return false;
		}

		if (bitmap.compressed) {
			texID = bitmap.CreateDDSTexture(texID);
		} else {
			if (resize) bitmap = bitmap.CreateRescaled(resizeDimensions.x, resizeDimensions.y);
			if (invert) bitmap.InvertColors();
			if (greyed) bitmap.MakeGrayScale();
			if (tint)   bitmap.Tint(tintColor);

			// const int xbits = count_bits_set(bitmap.xsize);
			// const int ybits = count_bits_set(bitmap.ysize);

			// make the texture
			glBindTexture(GL_TEXTURE_2D, texID);

			if (clamped) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			}

			if (nearest || linear) {
				constexpr GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f};

				if (border)
					glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);

				if (nearest) {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				} else {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bitmap.xsize, bitmap.ysize, int(border), GL_RGBA, GL_UNSIGNED_BYTE, bitmap.GetRawMem());
			} else {
				// MIPMAPPING (default)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

				glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, bitmap.xsize, bitmap.ysize, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.GetRawMem());
			}

			if (aniso)
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, globalRendering->maxTexAnisoLvl);
		}

		texInfo.id    = texID;
		texInfo.xsize = bitmap.xsize;
		texInfo.ysize = bitmap.ysize;

		GenInsertTex(texName, texInfo, false, false, true, false);
		return true;
	}

	static bool GenLoadTex(const std::string& texName)
	{
		GLuint texID = 0;
		glGenTextures(1, &texID);
		return (Load(texName, texID));
	}


	bool Bind(const std::string& texName)
	{
		if (texName.empty())
			return false;

		// cached
		const auto it = texInfoMap.find(texName);

		if (it != texInfoMap.end()) {
			const size_t texIdx = it->second;
			const GLuint texID = texInfoVec[texIdx].id;
			glBindTexture(GL_TEXTURE_2D, texID);
			return (texID != 0);
		}

		// load texture
		return (GenLoadTex(texName));
	}


	void Update()
	{
		if (waitingTextures.empty())
			return;

		const std::lock_guard<spring::recursive_mutex> lck(mutex);

		glAttribStatePtr->PushTextureBit();

		for (const std::string& texString: waitingTextures) {
			const auto mit = texInfoMap.find(texString);

			if (mit == texInfoMap.end())
				continue;

			Load(texString, texInfoVec[mit->second].id);
		}

		glAttribStatePtr->PopBits();
		waitingTextures.clear();
	}


	bool Free(const std::string& texName)
	{
		if (texName.empty())
			return false;

		return (EraseTex(texName));
	}


	size_t GetInfoIndex(const std::string& texName)
	{
		const auto it = texInfoMap.find(texName);

		if (it != texInfoMap.end())
			return (it->second);

		return (size_t(-1));
	}

	const TexInfo* GetInfo(size_t texIdx) { return &texInfoVec[texIdx]; }
	const TexInfo* GetInfo(const std::string& texName, bool forceLoad, bool persist)
	{
		if (texName.empty())
			return nullptr;

		const size_t texIdx = GetInfoIndex(texName);

		if (texIdx != size_t(-1)) {
			texInfoVec[texIdx].persist |= persist;
			return &texInfoVec[texIdx];
		}

		if (forceLoad) {
			// load texture
			GenLoadTex(texName);

			return &texInfoVec[ texInfoMap[texName] ];
		}

		return nullptr;
	}


	/******************************************************************************/

} // namespace CNamedTextures
