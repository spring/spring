/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CFONTTEXTURE_H
#define _CFONTTEXTURE_H
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <string>
#include <memory>
#include "Rendering/Textures/IAtlasAllocator.h"
#include "Rendering/Textures/RowAtlasAlloc.h"


struct FT_FaceRec_;
typedef struct FT_FaceRec_* FT_Face;
class CBitmap;
struct FontFace;


struct IGlyphRect { //FIXME use SRect
	IGlyphRect():
		x(0),y(0),
		w(0),h(0) {
	};

	IGlyphRect(float _x,float _y,float _w,float _h):
		x(_x),y(_y),
		w(_w),h(_h) {
	};

	float x0() const {
		return x;
	};
	float x1() const {
		return x+w;
	};
	float y0() const {
		return y;
	};
	float y1() const {
		return y+h;
	};

	float x,y;
	float w,h;
};

struct GlyphInfo {
	GlyphInfo()
	: advance(0)
	, height(0)
	, descender(0)
	, index(0)
	, utf16(0)
	, face(NULL)
	{ };

	IGlyphRect size;
	IGlyphRect texCord;
	IGlyphRect shadowTexCord;
	float advance, height, descender;
	char32_t index;
	char32_t utf16;
	FT_Face face;
};



/**
This class just store glyphs and load new glyphs if requred
It works with image and don't care about rendering these glyphs
It works only and only with UTF32 chars
**/
class CFontTexture
{
public:
	static void Update();

public:
	CFontTexture(const std::string& fontfile, int size, int outlinesize, float  outlineweight);
	virtual ~CFontTexture();

public:
	int GetSize() const { return fontSize; }
	int GetTextureWidth() const { return texWidth; }
	int GetTextureHeight() const { return texHeight; }
	int   GetOutlineWidth() const { return outlineSize; }
	float GetOutlineWeight() const { return outlineWeight; }
	float GetLineHeight() const { return lineHeight; }
	float GetDescender() const { return fontDescender; }
	int GetTexture() const { return texture; }
	const std::string& GetFamily()   const { return fontFamily; }
	const std::string& GetStyle()    const { return fontStyle; }

	const GlyphInfo& GetGlyph(char32_t ch); //< Get or load a glyph

protected:
	const FT_Face& GetFace() const { return face; }
	float GetKerning(const GlyphInfo& lgl,const GlyphInfo& rgl);
	void UpdateTexture();

private:
	void CreateTexture(const int width, const int height);

	// Load all chars in block's range
	void LoadBlock(char32_t start, char32_t end);
	void LoadGlyph(char32_t ch);

protected:
	std::unordered_map<char32_t, GlyphInfo> glyphs; // UTF16 -> GlyphInfo

	float kerningPrecached[128 * 128]; // contains ASCII kerning
	std::unordered_map<uint32_t, float> kerningDynamic; // contains unicode kerning

	int outlineSize;
	float outlineWeight;
	float lineHeight;
	float fontDescender;
	float normScale;
	int fontSize;
	std::string fontFamily;
	std::string fontStyle;
	int texWidth, texHeight;
	int wantedTexWidth, wantedTexHeight;
	unsigned int texture;

public:
	unsigned int textureSpaceMatrix;

private:
	CBitmap* atlasUpdate;
	CBitmap* atlasUpdateShadow;
	int lastTextureUpdate;
	int curTextureUpdate;

	FT_Face face;
	std::shared_ptr<FontFace> shFace;
	std::unordered_set<std::shared_ptr<FontFace>> usedFallbackFonts;

	CRowAtlasAlloc atlasAlloc;
};

#endif // CFONTTEXTURE_H
