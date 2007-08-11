#ifndef NAMED_TEXTURES_H
#define NAMED_TEXTURES_H
// NamedTextures.h: interface for the CNamedTextures class.
//
//////////////////////////////////////////////////////////////////////

#include <map>
#include <string>
using std::map;
using std::string;


class CNamedTextures {
	public:
		static void Init();
		static void Kill();

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
		static map<string, TexInfo> texMap;
};


#endif /* NAMED_TEXTURES_H */
