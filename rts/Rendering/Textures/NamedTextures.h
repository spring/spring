/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NAMED_TEXTURES_H
#define NAMED_TEXTURES_H

#include <map>
#include <vector>
#include <string>

using std::map;
using std::string;
using std::vector;

class CNamedTextures {
	public:
		static void Init();
		static void Kill();

		/**
		 * Reload textures we could not load because Bind() was called
		 * when compiling a DList.
		 * Otherwise, it would re-upload the texture-data on each call
		 * of the DList, so we delay it and load them here.
		 */
		static void Update();

		static bool Bind(const string& texName);
		static bool Free(const string& texName);

		struct TexInfo {
			TexInfo()
				: id(0), type(-1), xsize(-1), ysize(-1), alpha(false) {}
			unsigned int id;
			int type;
			int xsize;
			int ysize;
			bool alpha;
		};

		static const TexInfo* GetInfo(const string& texName);

	private:
		static bool Load(const string& texName, unsigned int texID);

		static map<string, TexInfo> texMap;
		static vector<string> texWaiting;
};

#endif /* NAMED_TEXTURES_H */
