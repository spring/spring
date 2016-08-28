/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// must be included before streflop! else we get streflop/cmath resolve conflicts in its hash implementation files
#include <boost/unordered_map.hpp>
#include <vector>
#include <mutex>

#include "NamedTextures.h"

#include "Rendering/GL/myGL.h"
#include "Bitmap.h"
#include "Rendering/GlobalRendering.h"
#include "System/bitops.h"
#include "System/TimeProfiler.h"
#include "System/type2.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringMutex.h"


#ifdef _MSC_VER
	#include <map>
	// only way to compile unordered_map with MSVC appears to require inclusion of math.h instead of streflop,
	// and that cannot be done here because it gives rise to other conflicts
	typedef std::map<std::string, CNamedTextures::TexInfo> TEXMAP;
#else
	typedef boost::unordered_map<std::string, CNamedTextures::TexInfo> TEXMAP;
#endif

namespace CNamedTextures {

	static TEXMAP texMap;
	static std::vector<std::string> texWaiting;
	static spring::recursive_mutex mutex;

	/******************************************************************************/

	void Init()
	{
		texMap.clear();
		texWaiting.clear();
	}

	void Kill(bool shutdown)
	{
		TEXMAP tempMap;

		const std::lock_guard<spring::recursive_mutex> lck(mutex);
		for (auto it = texMap.cbegin(); it != texMap.cend(); ++it) {
			if (shutdown || !it->second.persist) {
				const GLuint texID = it->second.id;
				glDeleteTextures(1, &texID);
			} else {
				tempMap[it->first] = it->second;
			}
		}

		std::swap(texMap, tempMap);
		texWaiting.clear();
	}


	/******************************************************************************/

	static bool Load(const std::string& texName, unsigned int texID)
	{
		//! strip off the qualifiers
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
			if (p < (int)filename.size()) {
				filename = filename.substr(p + 1);
			} else {
				filename.clear();
			}
		}

		//! get the image
		CBitmap bitmap;
		TexInfo texInfo;

		if (!bitmap.Load(filename)) {
			LOG_L(L_WARNING, "Couldn't find texture \"%s\"!", filename.c_str());
			const std::lock_guard<spring::recursive_mutex> lck(mutex);
			texMap[texName] = texInfo;
			glBindTexture(GL_TEXTURE_2D, 0);
			return false;
		}

		if (bitmap.compressed) {
			texID = bitmap.CreateDDSTexture(texID);
		} else {
			if (resize) bitmap = bitmap.CreateRescaled(resizeDimensions.x,resizeDimensions.y);
			if (invert) bitmap.InvertColors();
			if (greyed) bitmap.GrayScale();
			if (tint)   bitmap.Tint(tintColor);

			//! make the texture
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
					(!GLEW_ARB_texture_non_power_of_two || (globalRendering->atiHacks && nearest)) )
				{
					bitmap = bitmap.CreateRescaled(next_power_of_2(bitmap.xsize),next_power_of_2(bitmap.ysize));
				}

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
							bitmap.xsize, bitmap.ysize, border ? 1 : 0,
							GL_RGBA, GL_UNSIGNED_BYTE, &bitmap.mem[0]);
			} else {
				//! MIPMAPPING (default)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

				if ((count_bits_set(bitmap.xsize)==1 && count_bits_set(bitmap.ysize)==1) ||
					GLEW_ARB_texture_non_power_of_two)
				{
					glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, bitmap.xsize, bitmap.ysize,
								GL_RGBA, GL_UNSIGNED_BYTE, &bitmap.mem[0]);
				} else {
					//! glu auto resizes to next POT
					gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, bitmap.xsize, bitmap.ysize,
									GL_RGBA, GL_UNSIGNED_BYTE, &bitmap.mem[0]);
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
		texInfo.xsize = bitmap.xsize;
		texInfo.ysize = bitmap.ysize;

		const std::lock_guard<spring::recursive_mutex> lck(mutex);
		texMap[texName] = texInfo;
		return true;
	}


	bool Bind(const std::string& texName)
	{
		if (texName.empty()) {
			return false;
		}

		// cached
		const auto it = texMap.find(texName);
		if (it != texMap.end()) {
			const GLuint texID = it->second.id;
			glBindTexture(GL_TEXTURE_2D, texID);
			return (texID != 0);
		}

		// load texture
		GLboolean inListCompile;
		glGetBooleanv(GL_LIST_INDEX, &inListCompile);
		if (inListCompile) {
			const std::lock_guard<spring::recursive_mutex> lck(mutex);
			GLuint texID = 0;
			glGenTextures(1, &texID);

			TexInfo texInfo;
			texInfo.id = texID;
			texMap[texName] = texInfo;

			glBindTexture(GL_TEXTURE_2D, texID);

			texWaiting.push_back(texName);
			return true;
		}

		GLuint texID = 0;
		glGenTextures(1, &texID);
		return Load(texName, texID);
	}


	void Update()
	{
		if (texWaiting.empty()) {
			return;
		}

		const std::lock_guard<spring::recursive_mutex> lck(mutex);

		glPushAttrib(GL_TEXTURE_BIT);
		for (const std::string& texString: texWaiting) {
			const auto mit = texMap.find(texString);
			if (mit != texMap.end()) {
				Load(texString, mit->second.id);
			}
		}
		glPopAttrib();
		texWaiting.clear();
	}


	bool Free(const std::string& texName)
	{
		if (texName.empty()) {
			return false;
		}

		const std::lock_guard<spring::recursive_mutex> lck(mutex);
		const auto it = texMap.find(texName);
		if (it != texMap.end()) {
			const GLuint texID = it->second.id;
			glDeleteTextures(1, &texID);
			texMap.erase(it);
			return true;
		}
		return false;
	}


	const TexInfo* GetInfo(const std::string& texName, bool forceLoad, bool persist)
	{
		if (texName.empty()) {
			return NULL;
		}

		TEXMAP::iterator it = texMap.find(texName);
		if (it != texMap.end()) {
			it->second.persist |= persist;
			return &it->second;
		}

		if (forceLoad) {
			// load texture
			GLboolean inListCompile;
			glGetBooleanv(GL_LIST_INDEX, &inListCompile);
			if (inListCompile) {
				const std::lock_guard<spring::recursive_mutex> lck(mutex);
				GLuint texID = 0;
				glGenTextures(1, &texID);

				TexInfo texInfo;
				texInfo.id = texID;
				texInfo.persist = persist;
				texMap[texName] = texInfo;

				texWaiting.push_back(texName);
			} else {
				GLuint texID = 0;
				glGenTextures(1, &texID);
				Load(texName, texID);
			}
			return &texMap[texName];
		}

		return NULL;
	}


	/******************************************************************************/

} // namespace CNamedTextures
