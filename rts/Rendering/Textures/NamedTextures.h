/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NAMED_TEXTURES_H
#define NAMED_TEXTURES_H

#include <string>

namespace CNamedTextures {
	void Init();
	void Kill(bool shutdown = false);

	/**
	 * Reload textures we could not load because Bind() was called
	 * when compiling a DList.
	 * Otherwise, it would re-upload the texture-data on each call
	 * of the DList, so we delay it and load them here.
	 */
	void Update();

	bool Bind(const std::string& texName);
	bool Free(const std::string& texName);

	struct TexInfo {
		TexInfo()
			: id(0), xsize(-1), ysize(-1), alpha(false), persist(false) {}
		unsigned int id;
		int xsize;
		int ysize;
		bool alpha;
		bool persist;
	};

	size_t GetInfoIndex(const std::string& texName);

	const TexInfo* GetInfo(const std::string& texName, bool forceLoad = false, bool persist = false);
	const TexInfo* GetInfo(size_t texIdx);
}

#endif /* NAMED_TEXTURES_H */
